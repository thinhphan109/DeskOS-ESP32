# PC Monitor Pro — v2.5.0-dev

Native Windows Overview now includes CPU/GPU/RAM history for the last 60 seconds. It stores at most 61 one-second buckets, replaces duplicate updates within a bucket, rejects non-finite/out-of-range percentages, leaves unsupported readings empty, and breaks lines across gaps longer than 2.5 seconds. Source samples already aged 2.5 seconds are not repeated as new measurements. Changing host or DEMO mode clears the buffer.

The chart uses monotonic time and retains no persistent telemetry log. Existing real CPU/RAM/network, optional NVML/LHM GPU/temperature/VRAM readings, ESP history and Web charts remain in use. Temperature and fan support are provider-dependent; missing sensors remain N/A. Native preview is explicitly labelled DEMO and never connects to the board.

Automated coverage includes bucket replacement, gaps, null/NaN, mode/host isolation and storage bounds. Windows visual QA checks the compact Overview at the app's standard window size. Real sensor/driver acceptance remains specific to each PC.
