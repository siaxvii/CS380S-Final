import matplotlib.pyplot as plt
import pandas as pd

# load_times = {insert your load times here in dictionary format}
#Example data for demonstration (in nanoseconds): "python_audio_loading_benchmark/AUDIO/91/9.wav': 15288731"

# file_sizes = {insert your file sizes here in dictionary format}
# Example data for demonstration (in bytes): "python_audio_loading_benchmark/AUDIO/91/9.wav': 8026244"

sizes_kb = [size / 1024 for size in file_sizes.values()]
times_ns = list(load_times.values())

# Trim both lists to the smallest length
min_len = min(len(sizes_kb), len(times_ns))
sizes_kb = sizes_kb[:min_len]
times_ns = times_ns[:min_len]

# Assuming paired and sorted as before
paired = list(zip(sizes_kb, times_ns))
paired.sort(key=lambda x: x[1])  # sort by load time

# Create DataFrame
df = pd.DataFrame(paired, columns=['File Size (KB)', 'Load Time (ns)'])

# Save to CSV
df.to_csv('load_time_vs_size.csv', index=False)

print("CSV file saved successfully!")


import pandas as pd

# Load your original CSV file
df = pd.read_csv("formatted_output.csv")  # Replace with your actual file path if different

# Group by 'File Size (KB)' and calculate the average load time
averages = df.groupby("File Size (KB)")["Load Time (ns)"].mean().reset_index()

# Optionally rename the columns
averages.columns = ["File Size (KB)", "Average Load Time (ns)"]

# Save to new CSV
averages.to_csv("averaged_load_times.csv", index=False)

print("Averages calculated and saved to 'averaged_load_times.csv'.")
