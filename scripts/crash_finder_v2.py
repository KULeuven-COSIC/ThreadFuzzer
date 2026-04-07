import argparse
import re
from dataclasses import dataclass, field
from typing import Set, List, Tuple, Dict

@dataclass(frozen=True)
class Mutation:
    field: str
    new_val: str

@dataclass
class MutatedPacket:
    name: str
    packet_type: str
    mutations: Set[Mutation]
    iteration: int = -1

@dataclass
class EpochData:
    epoch_id: int
    start_reboots: int = 0
    end_reboots: int = 0
    normal_iterations: int = 0
    start_iteration: int = -1
    end_iteration: int = -1
    packets: List[MutatedPacket] = field(default_factory=list)

    @property
    def real_reboot_diff(self):
        return self.end_reboots - self.start_reboots

CRASH_SIGNATURES = [
    MutatedPacket("V1", "Child ID Response", {Mutation("thread_nwd.tlv.prefix.length", "New value=255")}),
    MutatedPacket("V1", "Data Response", {Mutation("thread_nwd.tlv.prefix.length", "New value=255")}), 
    MutatedPacket("V3", "Child ID Response", {Mutation("thread_nwd.tlv.len", "New value=255")}),
    MutatedPacket("V3", "Data Response", {Mutation("thread_nwd.tlv.len", "New value=255")}),
    MutatedPacket("V4", "Child ID Response", {
        Mutation("mle.tlv.addr16", "New value=65535"),
        Mutation("mle.tlv.source_addr", "New value=65535")
    }),
    MutatedPacket("V5", "Advertisement", {
        Mutation("mle.tlv.leader_data.router_id", "New value=255")
    }),
]

def parse_log_file(filepath: str) -> List[EpochData]:
    epochs = []
    current_epoch = EpochData(epoch_id=1)
    waiting_for_baseline = False
    waiting_for_final = False
    current_summary = ""
    current_mutations = set()
    current_iteration = -1 
    mutation_pattern = re.compile(r"Fuzzed field\s+([a-zA-Z0-9_.]+).*?(New value=[^;]+)")
    iteration_pattern = re.compile(r"START OF A NEW FUZZING ITERATION\s+(\d+)")
    reboot_pattern = re.compile(r"COLLECTED RBT CNT FOR NODE \d+:\s+(\d+)")
    try:
        with open(filepath, "r") as file:
            for line in file:
                if "CHIP pairing successful" in line:
                    waiting_for_baseline = True
                elif "Fetching current_reboot_count to check for crashes" in line:
                    waiting_for_final = True
                elif "COLLECTED RBT CNT" in line:
                    match = reboot_pattern.search(line)
                    if match:
                        val = int(match.group(1))
                        if waiting_for_baseline:
                            current_epoch.start_reboots = val
                            waiting_for_baseline = False
                        elif waiting_for_final:
                            current_epoch.end_reboots = val
                            epochs.append(current_epoch)
                            current_epoch = EpochData(epoch_id=len(epochs) + 1)
                            waiting_for_final = False
                elif "START OF A NEW FUZZING ITERATION" in line:
                    current_epoch.normal_iterations += 1
                    match = iteration_pattern.search(line)
                    if match:
                        current_iteration = int(match.group(1))
                        if current_epoch.start_iteration == -1:
                            current_epoch.start_iteration = current_iteration
                        current_epoch.end_iteration = current_iteration
                elif "---> Dissector's summary" in line:
                    if current_summary:
                        current_epoch.packets.append(MutatedPacket(
                            name="parsed_input", 
                            packet_type=current_summary, 
                            mutations=current_mutations,
                            iteration=current_iteration
                        ))
                    try:
                        current_summary = line.split("summary: ")[1].strip()
                    except IndexError:
                        current_summary = "Unknown"
                    current_mutations = set()
                elif "Mutator" in line and current_summary:
                    match = mutation_pattern.search(line)
                    if match:
                        field_name, new_val = match.groups()
                        current_mutations.add(Mutation(field_name, new_val))
    except FileNotFoundError:
        return []
    return epochs

def analyze_crashes(epochs: List[EpochData], active_signatures: List[MutatedPacket], device_type: str):
    if not epochs:
        print("\n[!] No valid log data found.")
        return
    first_occurrence = {}
    crash_counts = {f"{c.name} [{c.packet_type}]": 0 for c in active_signatures}
    total_unexpected_reboots = 0
    total_reboot_delta = 0
    total_iters = 0
    anomalies = []
    global_packet_index = 0 
    overall_first_seed = None
    for epoch in epochs:
        total_iters += epoch.normal_iterations
        total_reboot_delta += epoch.real_reboot_diff
        epoch_crash_count = 0
        for packet in epoch.packets:
            global_packet_index += 1
            for crash in active_signatures:
                if crash.packet_type in packet.packet_type and crash.mutations.issubset(packet.mutations):
                    key = f"{crash.name} [{crash.packet_type}]"
                    crash_counts[key] += 1
                    epoch_crash_count += 1
                    if key not in first_occurrence:
                        first_occurrence[key] = (global_packet_index, packet.iteration)
                    if overall_first_seed is None or global_packet_index < overall_first_seed[0]:
                        overall_first_seed = (global_packet_index, packet.iteration)
        expected_delta = epoch.normal_iterations + epoch_crash_count
        actual_delta = epoch.real_reboot_diff
        diff = actual_delta - expected_delta
        if diff > 0:
            total_unexpected_reboots += diff
            anomalies.append(f"Epoch {epoch.epoch_id} (Iters {epoch.start_iteration}-{epoch.end_iteration}): {diff} unexpected reboot(s)")
    total_crashes_global = total_reboot_delta - total_iters
    total_crash_mutations = sum(crash_counts.values())
    print(f"\n--- FUZZING ANALYSIS REPORT ({device_type}) ---")
    print("\n1. CRASH EXECUTION SUMMARY")
    print(f"   - Total Crashes Found: {max(0, total_crashes_global)}")
    print(f"   - Total Iterations:    {total_iters}")
    print(f"   - Total Reboot Delta:  {total_reboot_delta}")
    print("\n2. CRASHING MUTATIONS INFORMATION")
    print(f"   - Total Crash Mutations: {total_crash_mutations}")
    if overall_first_seed:
        print(f"   - First Crashing Mutation:   Packet #{overall_first_seed[0]} (Iteration {overall_first_seed[1]})")
    else:
        print("   - First Crashing Mutation:   None detected")
    print("\n   [Detailed Breakdown]")
    if not first_occurrence:
        print("   No signature-matched crashes found.")
    else:
        for k in sorted(crash_counts.keys()):
            occ = first_occurrence.get(k)
            occ_info = f"First: Packet #{occ[0]} (Iter {occ[1]})" if occ else "N/A"
            print(f"   - {k}: {crash_counts[k]} count | {occ_info}")
    print("\n3. UNEXPECTED REBOOTS")
    print(f"   - Total Anomalies: {total_unexpected_reboots}")
    if anomalies:
        for a in anomalies:
            print(f"   - [!] {a}")
    else:
        print("   - No anomalies detected.")
    print("\n" + "-"*31 + "\n")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("logfile")
    parser.add_argument("--device", "-d", choices=["MTD", "FTD"], required=True)
    args = parser.parse_args()
    mapping = {"MTD": ["V1", "V3", "V4"], "FTD": ["V1", "V3", "V5"]}
    active = [c for c in CRASH_SIGNATURES if c.name in mapping[args.device]]
    epochs = parse_log_file(args.logfile)
    analyze_crashes(epochs, active, args.device)

if __name__ == "__main__":
    main()