// SPDX-License-Identifier: GPL-3.0-or-later
//
// Server-side symbol resolution for crash backtraces.
// Fetches .sym files (nm -nC output) from R2 and resolves raw hex addresses
// to function names + offsets.

/** A single symbol from nm -nC output. */
export interface Symbol {
  address: number;
  type: string;
  name: string;
}

/** A resolved backtrace frame. */
export interface ResolvedFrame {
  raw: string;
  fileAddr?: string;
  symbol?: string;
}

/** Result of resolving a full backtrace. */
export interface ResolvedBacktrace {
  frames: ResolvedFrame[];
  resolvedRegisters?: Record<string, string>;
  loadBase?: string;
  autoDetectedBase: boolean;
  symbolFileFound: boolean;
}

/** Crash report fields used by the symbol resolver. */
export interface CrashReport {
  app_version?: string;
  platform?: string;
  app_platform?: string;
  load_base?: string;
  backtrace?: string[];
  registers?: Record<string, string>;
  signal?: number;
  signal_name?: string;
  fault_code_name?: string;
  fault_addr?: string;
  timestamp?: string;
  uptime_seconds?: number;
  ram_mb?: number;
  cpu_cores?: number;
  printer_model?: string;
  klipper_version?: string;
  display_backend?: string;
  log_tail?: string[];
}

/**
 * Parse `nm -nC` output into a sorted array of symbols.
 * Only includes text/code symbols (T/t/W/w types).
 */
export function parseSymbolTable(text: string): Symbol[] {
  const symbols: Symbol[] = [];
  for (const line of text.split("\n")) {
    if (!line.trim()) continue;

    // nm output format: "00000000004xxxxx T function_name"
    // With demangled names: "00000000004xxxxx T std::vector<int>::push_back(int const&)"
    const match = line.match(/^([0-9a-fA-F]+)\s+([A-Za-z])\s+(.+)$/);
    if (!match) continue;

    const [, addrHex, type, name] = match;

    // Only text/code symbols
    if (type !== "T" && type !== "t" && type !== "W" && type !== "w") continue;

    symbols.push({
      address: parseInt(addrHex, 16),
      type,
      name: name.trim(),
    });
  }

  return symbols;
}

/**
 * Binary search for the largest symbol address <= target.
 */
export function lookupSymbol(symbols: Symbol[], address: number): Symbol | null {
  if (symbols.length === 0) return null;

  let lo = 0;
  let hi = symbols.length - 1;

  // Address is before first symbol
  if (address < symbols[0].address) return null;

  while (lo <= hi) {
    const mid = (lo + hi) >>> 1;
    if (symbols[mid].address <= address) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }

  // hi is now the index of the largest symbol address <= target
  return symbols[hi];
}

/**
 * Auto-detect ASLR load base by matching _start/main gap in backtrace.
 * Port of auto_detect_load_base() from scripts/resolve-backtrace.sh.
 */
export function autoDetectLoadBase(symbols: Symbol[], backtraceAddrs: string[]): number | null {
  // Find _start and main in symbol table
  const startSym = symbols.find((s) => s.name === "_start");
  const mainSym = symbols.find((s) => s.name === "main");

  if (!startSym || !mainSym) return null;

  const expectedGap = startSym.address - mainSym.address;

  // Parse backtrace addresses to numbers
  const addrs = backtraceAddrs.map((a) => parseInt(a.replace(/^0x/i, ""), 16));

  // Try each pair: look for the _start/main gap
  for (let i = 0; i < addrs.length; i++) {
    for (let j = i + 1; j < addrs.length; j++) {
      const gap = addrs[j] - addrs[i];

      // Check if this pair matches main→_start gap
      if (gap === expectedGap) {
        // addrs[i] = main, addrs[j] = _start
        const candidateBase = addrs[i] - mainSym.address;
        if (candidateBase > 0) return candidateBase;
      }

      // Check reverse: addrs[i] = _start, addrs[j] = main
      const negGap = addrs[i] - addrs[j];
      if (negGap === expectedGap) {
        const candidateBase = addrs[j] - mainSym.address;
        if (candidateBase > 0) return candidateBase;
      }
    }
  }

  // Fallback: try matching individual addresses to _start or main
  for (const addr of addrs) {
    // Try as _start
    const baseFromStart = addr - startSym.address;
    if (baseFromStart > 0) {
      const mainRuntime = baseFromStart + mainSym.address;
      if (addrs.includes(mainRuntime)) return baseFromStart;
    }

    // Try as main
    const baseFromMain = addr - mainSym.address;
    if (baseFromMain > 0) {
      const startRuntime = baseFromMain + startSym.address;
      if (addrs.includes(startRuntime)) return baseFromMain;
    }
  }

  return null;
}

/**
 * Parse a hex address string to a number.
 */
function parseHexAddr(addr: string): number {
  return parseInt(addr.replace(/^0x/i, ""), 16);
}

/**
 * Resolve a single address against the symbol table.
 */
function resolveAddr(symbols: Symbol[], addr: number): string | null {
  const sym = lookupSymbol(symbols, addr);
  if (!sym) return null;
  const offset = addr - sym.address;
  return `${sym.name}+0x${offset.toString(16)}`;
}

/**
 * Resolve a crash backtrace using symbol files from R2.
 * Never throws — returns gracefully degraded results on any error.
 */
export async function resolveBacktrace(bucket: R2Bucket, report: CrashReport): Promise<ResolvedBacktrace> {
  const result: ResolvedBacktrace = {
    frames: [],
    resolvedRegisters: undefined,
    loadBase: undefined,
    autoDetectedBase: false,
    symbolFileFound: false,
  };

  try {
    // Determine version and platform
    const version = report.app_version;
    const platform = report.platform || report.app_platform;
    if (!version || !platform) return result;

    // Fetch symbol file from R2
    const symKey = `symbols/v${version}/${platform}.sym`;
    const symObj = await bucket.get(symKey);
    if (!symObj) return result;

    result.symbolFileFound = true;

    const symText = await symObj.text();
    const symbols = parseSymbolTable(symText);
    if (symbols.length === 0) return result;

    // Determine load base
    let loadBase = 0;
    let autoDetected = false;

    if (report.load_base) {
      loadBase = parseHexAddr(report.load_base);
    }

    // Try auto-detection if no load_base or load_base is 0
    if (loadBase === 0 && report.backtrace && report.backtrace.length > 0) {
      const detected = autoDetectLoadBase(symbols, report.backtrace);
      if (detected !== null && detected > 0) {
        loadBase = detected;
        autoDetected = true;
      }
    }

    if (loadBase > 0) {
      result.loadBase = `0x${loadBase.toString(16)}`;
      result.autoDetectedBase = autoDetected;
    }

    // Resolve backtrace frames
    if (report.backtrace && report.backtrace.length > 0) {
      result.frames = report.backtrace.map((raw) => {
        const frame: ResolvedFrame = { raw };
        try {
          const runtimeAddr = parseHexAddr(raw);
          const fileAddr = loadBase > 0 ? runtimeAddr - loadBase : runtimeAddr;
          frame.fileAddr = `0x${fileAddr.toString(16)}`;
          const resolved = resolveAddr(symbols, fileAddr);
          if (resolved) frame.symbol = resolved;
        } catch {
          // Skip unresolvable frames
        }
        return frame;
      });
    }

    // Resolve registers (PC and LR are most useful)
    if (report.registers) {
      const resolved: Record<string, string> = {};
      for (const [reg, val] of Object.entries(report.registers)) {
        if (!val) continue;
        try {
          const runtimeAddr = parseHexAddr(val);
          const fileAddr = loadBase > 0 ? runtimeAddr - loadBase : runtimeAddr;
          const sym = resolveAddr(symbols, fileAddr);
          if (sym) resolved[reg] = sym;
        } catch {
          // Skip unresolvable registers
        }
      }
      if (Object.keys(resolved).length > 0) {
        result.resolvedRegisters = resolved;
      }
    }
  } catch {
    // Never throw — return whatever we have so far
  }

  return result;
}
