#!/usr/bin/env python3
"""
Audio Comparison Visualization for SID Test Harness

Generates visual reports comparing audio output between SID engines:
- Waveform overlays
- Spectrogram comparisons
- Difference plots
- Metric summary charts
"""

import argparse
import os
import sys
import json
import warnings
warnings.filterwarnings('ignore')

import numpy as np
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from scipy import signal as scipy_signal
import wave
import struct
from pathlib import Path
from datetime import datetime


def load_wav(filepath):
    """Load a WAV file and return samples and metadata."""
    with wave.open(str(filepath), 'rb') as wav:
        n_channels = wav.getnchannels()
        samp_width = wav.getsampwidth()
        n_frames = wav.getnframes()
        framerate = wav.getframerate()

        frames = wav.readframes(n_frames)

        if samp_width == 2:  # 16-bit
            samples = np.array(struct.unpack('<{}h'.format(len(frames) // 2), frames), dtype=np.float32) / 32768.0
        elif samp_width == 4:  # 32-bit
            samples = np.array(struct.unpack('<{}i'.format(len(frames) // 4), frames), dtype=np.float32) / 2147483648.0
        else:
            return None, None

        # Convert to mono if stereo
        if n_channels > 1:
            samples = samples.reshape(-1, n_channels).mean(axis=1)

        return samples, framerate


def compute_spectrogram(samples, sample_rate, nperseg=1024, noverlap=512):
    """Compute spectrogram for audio samples."""
    f, t, Sxx = scipy_signal.spectrogram(
        samples,
        sample_rate,
        nperseg=nperseg,
        noverlap=noverlap,
        scaling='spectrum'
    )
    return f, t, Sxx


def compute_fft(samples, sample_rate, n_fft=4096):
    """Compute FFT for audio samples."""
    # Pad or trim to n_fft
    if len(samples) < n_fft:
        samples = np.pad(samples, (0, n_fft - len(samples)))
    else:
        samples = samples[:n_fft]

    fft = np.fft.rfft(samples * np.hanning(n_fft))
    freqs = np.fft.rfftfreq(n_fft, 1.0 / sample_rate)

    return freqs, np.abs(fft)


def calculate_snr(signal, noise):
    """Calculate Signal-to-Noise Ratio in dB."""
    signal_power = np.mean(signal ** 2)
    noise_power = np.mean(noise ** 2)

    if noise_power < 1e-15:
        return float('inf')

    return 10 * np.log10(signal_power / noise_power)


def calculate_correlation(x, y):
    """Calculate Pearson correlation coefficient."""
    x = x - np.mean(x)
    y = y - np.mean(y)

    numerator = np.sum(x * y)
    denominator = np.sqrt(np.sum(x ** 2) * np.sum(y ** 2))

    if denominator < 1e-15:
        return 0.0

    return numerator / denominator


def create_waveform_overlay(wav_files, labels, output_path, title="Waveform Comparison"):
    """Create waveform overlay plot."""
    fig, axes = plt.subplots(len(wav_files), 1, figsize=(14, 3 * len(wav_files)))

    if len(wav_files) == 1:
        axes = [axes]

    for i, (filepath, label) in enumerate(zip(wav_files, labels)):
        samples, sample_rate = load_wav(filepath)

        if samples is None:
            continue

        # Downsample for visualization
        if len(samples) > 50000:
            step = len(samples) // 50000
            vis_samples = samples[::step]
        else:
            vis_samples = samples

        time_axis = np.arange(len(vis_samples)) / sample_rate * step

        axes[i].plot(time_axis, vis_samples, linewidth=0.5)
        axes[i].set_title(f"{label} - {len(samples)} samples")
        axes[i].set_xlabel("Time (s)")
        axes[i].set_ylabel("Amplitude")
        axes[i].set_ylim(-1.05, 1.05)
        axes[i].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def create_spectrogram_comparison(wav_files, labels, output_path, title="Spectrogram Comparison"):
    """Create spectrogram comparison grid."""
    n_files = len(wav_files)
    fig, axes = plt.subplots(n_files, 1, figsize=(14, 4 * n_files))

    if n_files == 1:
        axes = [axes]

    for i, (filepath, label) in enumerate(zip(wav_files, labels)):
        samples, sample_rate = load_wav(filepath)

        if samples is None:
            continue

        # Compute spectrogram
        f, t, Sxx = compute_spectrogram(samples, sample_rate)

        # Convert to dB
        Sxx_db = 10 * np.log10(Sxx + 1e-10)

        im = axes[i].pcolormesh(t, f, Sxx_db, shading='gouraud', cmap='viridis', vmin=-80, vmax=-20)
        axes[i].set_title(f"{label}")
        axes[i].set_ylabel("Frequency (Hz)")
        axes[i].set_ylim(0, sample_rate / 2)

    plt.colorbar(im, ax=axes, label="Power (dB)")
    plt.xlabel("Time (s)")
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def create_difference_plot(wav_files, labels, output_path, title="Waveform Difference"):
    """Create difference plot comparing two waveforms."""
    if len(wav_files) != 2:
        print("Difference plot requires exactly 2 files")
        return

    samples_a, rate_a = load_wav(wav_files[0])
    samples_b, rate_b = load_wav(wav_files[1])

    if samples_a is None or samples_b is None:
        return

    # Resample if different sample rates
    if rate_a != rate_b:
        if rate_a > rate_b:
            samples_a = scipy_signal.resample(samples_a, int(len(samples_a) * rate_b / rate_a))
        else:
            samples_b = scipy_signal.resample(samples_b, int(len(samples_b) * rate_a / rate_b))

    # Align lengths
    min_len = min(len(samples_a), len(samples_b))
    samples_a = samples_a[:min_len]
    samples_b = samples_b[:min_len]

    # Calculate difference
    diff = samples_a - samples_b

    # Calculate metrics
    correlation = calculate_correlation(samples_a, samples_b)
    snr = calculate_snr(samples_a, diff)
    max_diff = np.max(np.abs(diff))
    mean_diff = np.mean(np.abs(diff))

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))

    # Top: Both waveforms
    time_axis = np.arange(min_len) / rate_a
    axes[0].plot(time_axis, samples_a, label=labels[0], linewidth=0.5, alpha=0.7)
    axes[0].plot(time_axis, samples_b, label=labels[1], linewidth=0.5, alpha=0.7)
    axes[0].set_title(f"Waveform Comparison")
    axes[0].set_ylabel("Amplitude")
    axes[0].set_ylim(-1.05, 1.05)
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Middle: Difference
    axes[1].plot(time_axis, diff, linewidth=0.5)
    axes[1].set_title(f"Difference (SNR: {snr:.1f} dB, Correlation: {correlation:.4f})")
    axes[1].set_ylabel("Amplitude Difference")
    axes[1].grid(True, alpha=0.3)

    # Bottom: Histogram of differences
    axes[2].hist(diff, bins=100, edgecolor='black', alpha=0.7)
    axes[2].set_title(f"Difference Distribution (Mean: {mean_diff:.4f}, Max: {max_diff:.4f})")
    axes[2].set_xlabel("Amplitude Difference")
    axes[2].set_ylabel("Count")
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def create_metric_chart(results, output_path, title="Engine Comparison Metrics"):
    """Create bar chart of comparison metrics."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    engines = list(results.keys())
    x = np.arange(len(engines))
    width = 0.25

    # Extract metrics for each engine
    correlations = []
    snrs = []
    max_diffs = []
    mean_diffs = []

    for engine in engines:
        engine_results = results[engine]
        if 'metrics' in engine_results:
            correlations.append(engine_results['metrics'].get('correlation', 0))
            snrs.append(engine_results['metrics'].get('snr_db', 0))
            max_diffs.append(engine_results['metrics'].get('max_diff', 0))
            mean_diffs.append(engine_results['metrics'].get('mean_diff', 0))
        else:
            correlations.append(0)
            snrs.append(0)
            max_diffs.append(0)
            mean_diffs.append(0)

    # Correlation plot (higher is better)
    axes[0, 0].bar(x, correlations, width=width)
    axes[0, 0].set_title("Correlation (closer to 1 = better)")
    axes[0, 0].set_ylabel("Correlation")
    axes[0, 0].set_xticks(x)
    axes[0, 0].set_xticklabels(engines, rotation=45, ha='right')
    axes[0, 0].set_ylim(0, 1.1)
    axes[0, 0].axhline(y=1.0, color='r', linestyle='--', alpha=0.5)
    axes[0, 0].grid(True, alpha=0.3, axis='y')

    # SNR plot (higher is better)
    axes[0, 1].bar(x, snrs, width=width, color='g')
    axes[0, 1].set_title("SNR (higher is better)")
    axes[0, 1].set_ylabel("SNR (dB)")
    axes[0, 1].set_xticks(x)
    axes[0, 1].set_xticklabels(engines, rotation=45, ha='right')
    axes[0, 1].grid(True, alpha=0.3, axis='y')

    # Max difference plot (lower is better)
    axes[1, 0].bar(x, max_diffs, width=width, color='orange')
    axes[1, 0].set_title("Max Difference (lower is better)")
    axes[1, 0].set_ylabel("Amplitude")
    axes[1, 0].set_xticks(x)
    axes[1, 0].set_xticklabels(engines, rotation=45, ha='right')
    axes[1, 0].grid(True, alpha=0.3, axis='y')

    # Mean difference plot (lower is better)
    axes[1, 1].bar(x, mean_diffs, width=width, color='purple')
    axes[1, 1].set_title("Mean Difference (lower is better)")
    axes[1, 1].set_ylabel("Amplitude")
    axes[1, 1].set_xticks(x)
    axes[1, 1].set_xticklabels(engines, rotation=45, ha='right')
    axes[1, 1].grid(True, alpha=0.3, axis='y')

    plt.suptitle(title, fontsize=14, y=1.02)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def create_fft_comparison(wav_files, labels, output_path, title="FFT Comparison"):
    """Create FFT magnitude comparison."""
    fig, axes = plt.subplots(2, 1, figsize=(14, 10))

    colors = ['blue', 'red', 'green', 'orange', 'purple']

    for i, (filepath, label) in enumerate(zip(wav_files, labels)):
        samples, sample_rate = load_wav(filepath)

        if samples is None:
            continue

        freqs, fft_mag = compute_fft(samples, sample_rate)

        axes[0].semilogy(freqs, fft_mag, label=label, color=colors[i % len(colors)], linewidth=0.8)
        axes[1].plot(freqs, 20 * np.log10(fft_mag + 1e-10), label=label, color=colors[i % len(colors)], linewidth=0.8)

    axes[0].set_title("FFT Magnitude")
    axes[0].set_xlabel("Frequency (Hz)")
    axes[0].set_ylabel("Magnitude")
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    axes[1].set_title("FFT Magnitude (dB)")
    axes[1].set_xlabel("Frequency (Hz)")
    axes[1].set_ylabel("Magnitude (dB)")
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def generate_full_report(results_dir, engines):
    """Generate all comparison reports."""
    results_dir = Path(results_dir)

    # Load comparison results if available
    compare_file = results_dir / "comparison.json"
    if compare_file.exists():
        with open(compare_file) as f:
            comparison_results = json.load(f)
    else:
        comparison_results = {}

    # Create output subdirectory
    output_dir = results_dir / "figures"
    output_dir.mkdir(exist_ok=True)

    print(f"\nGenerating visual reports to: {output_dir}")

    # Find all WAV files
    wav_files = {}
    for engine in engines:
        engine_dir = results_dir / engine
        if engine_dir.exists():
            wav_files[engine] = list(engine_dir.glob("*.wav"))

    # Create waveform comparisons for matching files
    common_files = set()
    for engine_files in wav_files.values():
        common_files.update(engine_files)

    if len(engines) >= 2:
        # Create difference plots for matching files - compare all engine pairs
        for i, engine_a in enumerate(engines):
            for j, engine_b in enumerate(engines):
                if i < j:  # Only compare each pair once
                    for file_a in wav_files.get(engine_a, []):
                        file_b = results_dir / engine_b / file_a.name
                        if file_b.exists():
                            output_name = f"difference_{engine_a}_vs_{engine_b}_{file_a.stem}.png"
                            create_difference_plot(
                                [str(file_a), str(file_b)],
                                [engine_a, engine_b],
                                str(output_dir / output_name)
                            )
                            print(f"  Created: {output_name}")

        # Create spectrogram comparison
        if wav_files.get(engines[0]):
            first_file = wav_files[engines[0]][0]
            other_files = []

            for engine in engines[1:]:
                file = results_dir / engine / first_file.name
                if file.exists():
                    other_files.append(str(file))

            if other_files:
                create_spectrogram_comparison(
                    [str(first_file)] + other_files,
                    engines,
                    str(output_dir / "spectrogram_comparison.png")
                )
                print(f"  Created: spectrogram_comparison.png")

    # Create metric chart
    if comparison_results:
        create_metric_chart(
            comparison_results,
            str(output_dir / "metrics_chart.png"),
            "Engine Comparison Metrics"
        )
        print(f"  Created: metrics_chart.png")

    # Generate HTML summary
    html_content = generate_html_summary(results_dir, engines, comparison_results)
    with open(results_dir / "index.html", 'w') as f:
        f.write(html_content)
    print(f"  Created: index.html")


def generate_html_summary(results_dir, engines, comparison_results):
    """Generate HTML summary page."""
    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>SID Engine Comparison Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        h1 {{ color: #333; }}
        h2 {{ color: #666; border-bottom: 1px solid #ddd; padding-bottom: 10px; }}
        .engine-section {{ margin: 20px 0; padding: 15px; background: #f9f9f9; border-radius: 5px; }}
        table {{ border-collapse: collapse; width: 100%; margin: 10px 0; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background: #4CAF50; color: white; }}
        tr:nth-child(even) {{ background: #f2f2f2; }}
        img {{ max-width: 100%; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; }}
        .metrics {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 10px; }}
        .metric {{ padding: 10px; background: #fff; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .metric h3 {{ margin: 0 0 10px 0; color: #4CAF50; }}
        .metric-value {{ font-size: 24px; font-weight: bold; color: #333; }}
        .metric-label {{ font-size: 12px; color: #666; }}
    </style>
</head>
<body>
    <h1>SID Engine Comparison Report</h1>
    <p>Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>

    <h2>Engines Tested</h2>
    <ul>
        {''.join(f'<li>{engine}</li>' for engine in engines)}
    </ul>

    <h2>Comparison Metrics</h2>
    <div class="metrics">
"""

    # Add metrics for each engine
    for engine, results in comparison_results.items():
        if 'metrics' in results:
            metrics = results['metrics']
            html += f"""
        <div class="metric">
            <h3>{engine}</h3>
            <div class="metric-value">{metrics.get('correlation', 0):.4f}</div>
            <div class="metric-label">Correlation</div>
        </div>
        <div class="metric">
            <h3>{engine}</h3>
            <div class="metric-value">{metrics.get('snr_db', 0):.1f}</div>
            <div class="metric-label">SNR (dB)</div>
        </div>
"""
        else:
            html += f"""
        <div class="metric">
            <h3>{engine}</h3>
            <div class="metric-value">-</div>
            <div class="metric-label">No comparison data</div>
        </div>
"""

    html += """
    </div>

    <h2>Waveform Comparisons</h2>
"""

    # Add difference plots
    figures_dir = results_dir / "figures"
    if figures_dir.exists():
        # Group difference plots by test type for better organization
        test_groups = {}
        for img in sorted(figures_dir.glob("difference_*.png")):
            # Extract test type from filename: difference_engine1_vs_engine2_testname.wav.png
            name = img.name.replace('.png', '')
            # Parse: difference_resid-0.16_vs_libresidfp_osc_Triangle
            parts = name.replace('difference_', '').split('_vs_')
            if len(parts) >= 2:
                engines_part = parts[0]
                test_part = '_'.join(parts[1:])
                if test_part not in test_groups:
                    test_groups[test_part] = []
                test_groups[test_part].append((img.name, engines_part))
        
        for test_name, plots in test_groups.items():
            html += f"""
    <h3>{test_name.replace('_', ' ').title()}</h3>
    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 10px;">
"""
            for img_name, engines in plots:
                html += f"""
        <div class="engine-section">
            <img src="figures/{img_name}" alt="{img_name}">
            <p>Comparison: {engines.replace('_', ' ')}</p>
        </div>
"""
            html += """
    </div>
"""

    html += """
    <h2>Spectrogram Comparison</h2>
    <div class="engine-section">
"""

    # Add spectrogram if exists
    if (figures_dir / "spectrogram_comparison.png").exists():
        html += """
        <img src="figures/spectrogram_comparison.png" alt="Spectrogram Comparison">
"""

    html += """
    </div>

    <h2>Summary</h2>
    <p>The comparison above shows the differences between SID engine audio outputs.</p>
    <p>Higher correlation values indicate more similar audio output.</p>
    <p>Higher SNR values indicate less distortion/difference.</p>

</body>
</html>
"""
    return html


def main():
    parser = argparse.ArgumentParser(
        description="Visualize SID Engine Comparison Results"
    )
    parser.add_argument(
        "results_dir",
        type=Path,
        help="Directory containing test results"
    )
    parser.add_argument(
        "--engines",
        nargs="+",
        default=["resid-0.16", "resid-1.0", "libresidfp"],
        help="Engine names to compare"
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Generate all visualizations"
    )

    args = parser.parse_args()

    if args.all:
        generate_full_report(args.results_dir, args.engines)
    else:
        print("Specify --all to generate all visualizations")
        print("Usage: python visualize_comparison.py <results_dir> --all")

    return 0


if __name__ == "__main__":
    sys.exit(main())