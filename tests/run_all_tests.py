#!/usr/bin/env python3
"""
SID Test Harness - Run tests for all SID engines and compare results

This script:
1. Builds the test harness for each SID engine (resid-0.16, resid-1.0, libresidfp)
2. Runs the tests and exports audio files
3. Compares results across engines
4. Generates a summary report
"""

import os
import sys
import subprocess
import shutil
import json
from pathlib import Path
from datetime import datetime
import argparse

# Configuration
BUILD_DIR = Path("cmake-build-debug")
TESTS_DIR = Path("tests")
OUTPUT_BASE = Path("test_results")
SID_ENGINES = {
    "resid-0.16": 0,
    "resid-1.0": 1,
    "libresidfp": 2,
}


def run_command(cmd, cwd=None, shell=False):
    """Run a command and return the result."""
    print(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    result = subprocess.run(
        cmd,
        cwd=cwd,
        shell=shell,
        capture_output=True,
        text=True
    )
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    return result


def build_test_harness(sid_engine_id, engine_name):
    """Build the test harness for a specific SID engine."""
    print(f"\n{'='*60}")
    print(f"Building test harness for {engine_name}...")
    print(f"{'='*60}")

    build_dir = BUILD_DIR / "tests" / engine_name
    build_dir.mkdir(parents=True, exist_ok=True)

    # Configure with the specific engine
    cmake_cmd = [
        "cmake",
        "-S", ".",
        "-B", str(build_dir),
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DSID_EMULATOR={sid_engine_id}",
    ]

    result = run_command(cmake_cmd)
    if result.returncode != 0:
        print(f"Failed to configure {engine_name} build")
        return None

    # Build the test target
    build_cmd = [
        "cmake",
        "--build", str(build_dir),
        "--target", "SID_tests",
        "--config", "Release",
        "-j", str(os.cpu_count())
    ]

    result = run_command(build_cmd)
    if result.returncode != 0:
        print(f"Failed to build {engine_name} test harness")
        return None

    # Find the built executable
    test_exe = build_dir / "SID_tests"
    if not test_exe.exists():
        # Try other possible locations
        test_exe = build_dir / "SID_tests.exe"
    if not test_exe.exists():
        # Check within the build directory structure
        for exe in build_dir.rglob("SID_tests*"):
            if exe.is_file():
                test_exe = exe
                break

    if not test_exe.exists():
        print(f"Could not find test executable for {engine_name}")
        return None

    # Return absolute path to avoid cwd issues
    return test_exe.resolve()


def run_engine_tests(test_exe, output_dir):
    """Run all tests for an engine and export audio."""
    print(f"\n{'='*60}")
    print(f"Running tests for {test_exe.parent.name}...")
    print(f"{'='*60}")

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Run tests without cwd - use absolute path for output directory
    result = run_command(
        [str(test_exe), "--output-dir", str(output_dir.resolve()), "test", "all"]
    )

    return result.returncode == 0


def compare_audio_files(dir_a, dir_b, output_file=None):
    """
    Compare audio files between two directories.
    Returns comparison metrics.
    """
    import wave
    import struct

    results = {}

    dir_a = Path(dir_a)
    dir_b = Path(dir_b)

    # Find all WAV files in directory A
    files_a = list(dir_a.glob("*.wav"))

    for file_a in files_a:
        file_b = dir_b / file_a.name

        if not file_b.exists():
            print(f"Warning: {file_b.name} not found in directory B")
            continue

        # Compare files
        with wave.open(str(file_a), 'rb') as wav_a:
            with wave.open(str(file_b), 'rb') as wav_b:
                # Get audio parameters
                n_channels = wav_a.getnchannels()
                samp_width = wav_a.getsampwidth()
                n_frames = wav_a.getnframes()
                framerate = wav_a.getframerate()

                if (wav_b.getnchannels() != n_channels or
                    wav_b.getnframes() != n_frames or
                    wav_b.getframerate() != framerate):
                    print(f"Warning: {file_a.name} has different parameters in B")
                    continue

                # Read audio data
                frames_a = wav_a.readframes(n_frames)
                frames_b = wav_b.readframes(n_frames)

                # Convert to samples
                if samp_width == 2:  # 16-bit
                    samples_a = struct.unpack('<{}h'.format(len(frames_a) // 2), frames_a)
                    samples_b = struct.unpack('<{}h'.format(len(frames_b) // 2), frames_b)
                else:
                    continue

                # Calculate metrics
                n = len(samples_a)
                sum_a = sum(samples_a)
                sum_b = sum(samples_b)
                sum_a_sq = sum(x*x for x in samples_a)
                sum_b_sq = sum(x*x for x in samples_b)
                sum_ab = sum(a*b for a, b in zip(samples_a, samples_b))
                sum_diff_sq = sum((a-b)**2 for a, b in zip(samples_a, samples_b))

                # Pearson correlation
                mean_a = sum_a / n
                mean_b = sum_b / n

                cov = (sum_ab / n) - (mean_a * mean_b)
                std_a = ((sum_a_sq / n) - (mean_a ** 2)) ** 0.5
                std_b = ((sum_b_sq / n) - (mean_b ** 2)) ** 0.5

                if std_a > 0 and std_b > 0:
                    correlation = cov / (std_a * std_b)
                else:
                    correlation = 0

                # SNR
                signal_power = sum_a_sq / n
                noise_power = sum_diff_sq / n

                if noise_power > 0 and signal_power > 0:
                    snr = 10 * __import__('math').log10(signal_power / noise_power)
                elif signal_power > 0:
                    snr = float('inf')  # No noise, perfect signal
                else:
                    snr = 0.0  # No signal, can't compute SNR

                # Max and mean difference
                max_diff = max(abs(a - b) for a, b in zip(samples_a, samples_b))
                mean_diff = sum(abs(a - b) for a, b in zip(samples_a, samples_b)) / n

                results[file_a.name] = {
                    "correlation": round(correlation, 6),
                    "snr_db": round(snr, 2),
                    "max_diff": round(max_diff, 6),
                    "mean_diff": round(mean_diff, 6),
                    "n_frames": n_frames
                }

    return results


def generate_report(results_dir, all_results):
    """Generate a summary report."""
    report = {
        "timestamp": datetime.now().isoformat(),
        "engines": list(SID_ENGINES.keys()),
        "results": all_results
    }

    report_file = results_dir / "comparison_report.json"
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)

    print(f"\n{'='*60}")
    print("SUMMARY REPORT")
    print(f"{'='*60}")

    for engine_name, engine_results in all_results.items():
        print(f"\n{engine_name}:")
        if isinstance(engine_results, dict):
            for key, value in engine_results.items():
                print(f"  {key}: {value}")
        else:
            print(f"  {engine_results}")

    print(f"\nFull report saved to: {report_file}")

    return report


def main():
    parser = argparse.ArgumentParser(
        description="SID Test Harness - Compare audio output of different SID engines"
    )
    parser.add_argument(
        "--engines",
        nargs="+",
        default=["resid-0.16", "resid-1.0", "libresidfp"],
        choices=list(SID_ENGINES.keys()),
        help="Engines to test (default: all)"
    )
    parser.add_argument(
        "--output-dir",
        default=OUTPUT_BASE,
        type=Path,
        help="Output directory for test results"
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip building and use existing executables"
    )

    args = parser.parse_args()

    results_dir = args.output_dir
    results_dir.mkdir(parents=True, exist_ok=True)

    print(f"SID Test Harness")
    print(f"Output directory: {results_dir}")
    print(f"Engines to test: {', '.join(args.engines)}")

    all_results = {}
    test_executables = {}

    # Build or find test executables
    for engine_name in args.engines:
        sid_id = SID_ENGINES[engine_name]

        if args.skip_build:
            # Try to find existing executable
            test_exe = BUILD_DIR / "SID_tests"
            if not test_exe.exists():
                print(f"Warning: --skip-build specified but {test_exe} not found")
                continue
            # Use absolute path
            test_exe = test_exe.resolve()
        else:
            test_exe = build_test_harness(sid_id, engine_name)
            if not test_exe:
                continue

        test_executables[engine_name] = test_exe

    # Run tests for each engine
    for engine_name, test_exe in test_executables.items():
        engine_output = results_dir / engine_name
        success = run_engine_tests(test_exe, engine_output)

        if not success:
            print(f"Warning: Tests failed for {engine_name}")

        # Collect results (simplified - would need actual metrics from test output)
        all_results[engine_name] = {
            "status": "passed" if success else "failed",
            "output_dir": str(engine_output)
        }

    # Compare engines
    engine_list = list(test_executables.keys())
    if len(engine_list) >= 2:
        print(f"\n{'='*60}")
        print("Comparing engines...")
        print(f"{'='*60}")

        # Compare all engine pairs
        all_comparisons = {}
        engine_metrics = {}  # Store metrics per engine for visualization

        for i, engine_a in enumerate(engine_list):
            for j, engine_b in enumerate(engine_list):
                if i < j:  # Only compare each pair once (A vs B, not B vs A)
                    print(f"\nComparing {engine_a} vs {engine_b}...")
                    comp_result = compare_audio_files(
                        results_dir / engine_a,
                        results_dir / engine_b
                    )
                    pair_key = f"{engine_a}_vs_{engine_b}"
                    all_comparisons[pair_key] = comp_result

                    # Calculate average metrics for this pair
                    if comp_result:
                        avg_snr = sum(r.get('snr_db', 0) for r in comp_result.values()) / len(comp_result)
                        avg_corr = sum(r.get('correlation', 0) for r in comp_result.values()) / len(comp_result)

                        # Store metrics for both engines in this pair
                        for engine in [engine_a, engine_b]:
                            if engine not in engine_metrics:
                                engine_metrics[engine] = {'snr_db': [], 'correlation': [], 'count': 0}
                            engine_metrics[engine]['snr_db'].append(avg_snr)
                            engine_metrics[engine]['correlation'].append(avg_corr)
                            engine_metrics[engine]['count'] += 1

        # Generate comparison.json with per-engine metrics for visualization
        # The visualization script expects: results[engine]['metrics'] structure
        comparison_output = {}

        # First, compute per-engine metrics
        for engine, metrics in engine_metrics.items():
            comparison_output[engine] = {
                'metrics': {
                    'correlation': round(sum(metrics['correlation']) / len(metrics['correlation']), 6),
                    'snr_db': round(sum(metrics['snr_db']) / len(metrics['snr_db']), 2),
                    'max_diff': 0.0,
                    'mean_diff': 0.0
                },
                'files_compared': sum(len(r) for k, r in all_comparisons.items() if engine in k and isinstance(r, dict))
            }

        # Add pairwise comparison details to each engine
        for pair_key, comp_result in all_comparisons.items():
            engines_in_pair = pair_key.split('_vs_')
            for engine in engines_in_pair:
                if engine in comparison_output:
                    if 'pairwise_comparisons' not in comparison_output[engine]:
                        comparison_output[engine]['pairwise_comparisons'] = {}
                    comparison_output[engine]['pairwise_comparisons'][pair_key] = {
                        'files_compared': len(comp_result) if comp_result else 0
                    }

        compare_file = results_dir / "comparison.json"
        with open(compare_file, 'w') as f:
            json.dump(comparison_output, f, indent=2)

        print(f"\nComparison saved to: {compare_file}")

        # Print summary of all comparisons
        print(f"\n{'='*60}")
        print("COMPARISON SUMMARY")
        print(f"{'='*60}")
        for pair_key, results in all_comparisons.items():
            if results:
                avg_snr = sum(r.get('snr_db', 0) for r in results.values()) / len(results)
                avg_corr = sum(r.get('correlation', 0) for r in results.values()) / len(results)
                print(f"\n{pair_key}:")
                print(f"  Files compared: {len(results)}")
                print(f"  Avg SNR: {avg_snr:.2f} dB")
                print(f"  Avg Correlation: {avg_corr:.6f}")
            else:
                print(f"\n{pair_key}: No matching files found")

    # Generate report
    report = generate_report(results_dir, all_results)

    # Generate visualizations
    print(f"\n{'='*60}")
    print("Generating visualizations...")
    print(f"{'='*60}")

    viz_cmd = [
        sys.executable,
        str(TESTS_DIR / "visualize_comparison.py"),
        str(results_dir.resolve()),
        "--engines"
    ] + args.engines + ["--all"]

    viz_result = run_command(viz_cmd)
    if viz_result.returncode != 0:
        print("Warning: Visualization generation had issues (check visualize_comparison.py)")
    else:
        print("Visualizations generated successfully!")

    print("\nDone!")

    return 0


if __name__ == "__main__":
    sys.exit(main())