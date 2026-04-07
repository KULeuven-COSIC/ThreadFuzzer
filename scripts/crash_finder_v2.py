import argparse
import re
import sys
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
    end_timestamp: str = "Unknown"
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

def parse_log_file(filepath: str) -> Tuple[List[EpochData], List[str]]:
    epochs = []
    broken_epochs_info = []
    current_epoch = EpochData(epoch_id=1)
    waiting_for_baseline = False
    waiting_for_final = False
    current_summary = ""
    current_mutations = set()
    current_iteration = -1 
    
    mutation_pattern = re.compile(r"Fuzzed field\s+([a-zA-Z0-9_.]+).*?(New value=[^;]+)")
    iteration_pattern = re.compile(r"START OF A NEW FUZZING ITERATION\s+(\d+)")
    reboot_pattern = re.compile(r"COLLECTED RBT CNT FOR NODE \d+:\s+(\d+)")
    timestamp_pattern = re.compile(r"^\[(.*?)\]")
    
    try:
        with open(filepath, "r") as file:
            for line in file:
                if "Failed to parse reboot count for Node" in line:
                    ts_match = timestamp_pattern.search(line)
                    ts = ts_match.group(1) if ts_match else "Unknown Timestamp"
                    broken_epochs_info.append(f"Attempted Epoch {len(epochs) + 1} at [{ts}]")
                    
                    current_epoch = EpochData(epoch_id=len(epochs) + 1)
                    waiting_for_baseline = False
                    waiting_for_final = False
                    current_summary = ""
                    current_mutations = set()
                    current_iteration = -1
                    continue

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
                            ts_match = timestamp_pattern.search(line)
                            if ts_match:
                                current_epoch.end_timestamp = ts_match.group(1)
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
        print(f"Error: File {filepath} not found.")
        sys.exit(1)
    return epochs, broken_epochs_info

def analyze_crashes(epochs: List[EpochData], active_signatures: List[MutatedPacket], device_type: str, broken_epochs: List[str], verbose_level: int):
    if not epochs and not broken_epochs:
        print(f"\n[!] No log data parsed for {device_type}.")
        return

    first_occurrence = {}
    crash_counts = {f"{c.name} [{c.packet_type}]": 0 for c in active_signatures}
    total_unexpected_reboots = 0
    total_reboot_delta = 0
    total_iters = 0
    anomalies = []
    iter_counts = []
    crashed_epochs_verbose = []
    non_crashing_signature_epochs = []
    global_packet_index = 0 
    overall_first_seed = None

    for epoch in epochs:
        iter_counts.append(epoch.normal_iterations)
        total_iters += epoch.normal_iterations
        total_reboot_delta += epoch.real_reboot_diff
        epoch_crash_count = 0
        epoch_triggered_signatures = set()
        
        for packet in epoch.packets:
            global_packet_index += 1
            for crash in active_signatures:
                if crash.packet_type in packet.packet_type and crash.mutations.issubset(packet.mutations):
                    key = f"{crash.name} [{crash.packet_type}]"
                    crash_counts[key] += 1
                    epoch_crash_count += 1
                    epoch_triggered_signatures.add(f"{key} (Iter. {packet.iteration})")
                    if key not in first_occurrence:
                        first_occurrence[key] = (global_packet_index, packet.iteration)
                    if overall_first_seed is None or global_packet_index < overall_first_seed[0]:
                        overall_first_seed = (global_packet_index, packet.iteration)

        epoch_total_crashes = epoch.real_reboot_diff - epoch.normal_iterations
        if epoch_total_crashes > 0:
            crashed_epochs_verbose.append({
                "info": f"Epoch {epoch.epoch_id} [{epoch.end_timestamp}] (Iters {epoch.start_iteration}-{epoch.end_iteration}): {epoch_total_crashes} crash(es) | RBT Cnt: {epoch.start_reboots} -> {epoch.end_reboots} (Delta: {epoch.real_reboot_diff}, Expected: {epoch.normal_iterations})",
                "signatures": sorted(list(epoch_triggered_signatures))
            })
        elif epoch_triggered_signatures:
            non_crashing_signature_epochs.append({
                "info": f"Epoch {epoch.epoch_id} [{epoch.end_timestamp}] (Iters {epoch.start_iteration}-{epoch.end_iteration}) | RBT Cnt: {epoch.start_reboots} -> {epoch.end_reboots} (Delta: {epoch.real_reboot_diff}, Expected: {epoch.normal_iterations})",
                "signatures": sorted(list(epoch_triggered_signatures))
            })

        expected_delta = epoch.normal_iterations + epoch_crash_count
        actual_delta = epoch.real_reboot_diff
        diff = actual_delta - expected_delta
        if diff > 0:
            total_unexpected_reboots += diff
            iter_range = f"Iters {epoch.start_iteration}-{epoch.end_iteration}"
            anomalies.append(f"Epoch {epoch.epoch_id} ({iter_range}): {diff} unexpected reboot(s)")

    total_crashes_global = total_reboot_delta - total_iters
    total_crash_mutations = sum(crash_counts.values())

    print(f"\n--- FUZZING ANALYSIS REPORT ({device_type} TARGET) ---")
    
    print("\n1. CAMPAIGN CRASH SUMMARY")
    print(f"   - Total Crashes Found: {max(0, total_crashes_global)}")
    print(f"   - Total Iterations:    {total_iters}")
    print(f"   - Total Reboot Delta:  {total_reboot_delta}")

    print("\n2. CRASHING SEEDS INFORMATION")
    print(f"   - Total Crash Mutations: {total_crash_mutations}")
    if overall_first_seed:
        print(f"   - First Crashing Seed:   Packet #{overall_first_seed[0]} (Iteration {overall_first_seed[1]})")
    else:
        print("   - First Crashing Seed:   None detected")
    
    print("\n   [Detailed Breakdown]")
    if not first_occurrence:
        print("   No signature-matched crashes found.")
    else:
        for k in sorted(crash_counts.keys()):
            occ = first_occurrence.get(k)
            occ_info = f"First at Packet #{occ[0]} (Iter {occ[1]})" if occ else "N/A"
            print(f"   - {k}: {crash_counts[k]} count | {occ_info}")

    print("\n3. UNEXPECTED REBOOTS (ANOMALIES)")
    print(f"   - Total Anomalies: {total_unexpected_reboots}")
    if anomalies:
        for a in anomalies:
            print(f"   - [!] {a}")
    else:
        print("   - No anomalies detected.")

    print("\n4. EPOCH SIZE STATISTICS")
    print(f"   - Total Broken Epochs:    {len(broken_epochs)}")
    if iter_counts:
        avg_iters = sum(iter_counts) / len(iter_counts)
        print(f"   - Total Epochs Validated: {len(epochs)}")
        print(f"   - Iterations per Epoch:   Min: {min(iter_counts)} | Max: {max(iter_counts)} | Avg: {avg_iters:.2f}")
    else:
        print("   - No valid iterations tracked.")

    if verbose_level >= 1:
        print("\n5. VERBOSE DETAILS")
        print("   [Broken Epochs]")
        if broken_epochs:
            for b in broken_epochs:
                print(f"   - Failed {b}")
        else:
            print("   - No broken epochs.")

        print("\n   [Detailed Crash Locations]")
        if crashed_epochs_verbose:
            for c_dict in crashed_epochs_verbose:
                print(f"   - {c_dict['info']}")
                if verbose_level >= 2:
                    if c_dict['signatures']:
                        print(f"       -> Signatures injected: {', '.join(c_dict['signatures'])}")
                    else:
                        print("       -> Signatures injected: NONE (Potential Unknown Anomaly)")
        else:
            print("   - No crashes detected in any epoch.")

        if verbose_level >= 2:
            print("\n   [Signatures Injected but NO Crash Observed]")
            if non_crashing_signature_epochs:
                for nc_dict in non_crashing_signature_epochs:
                    print(f"   - {nc_dict['info']}")
                    print(f"       -> Signatures injected: {', '.join(nc_dict['signatures'])}")
            else:
                print("   - None. All injected signatures correctly resulted in a crash.")

    print("\n" + "-"*35 + "\n")

def main():
    parser = argparse.ArgumentParser(description="Production-ready fuzzer log analysis script.")
    parser.add_argument("logfile", help="Path to the fuzzer log file.")
    parser.add_argument("--device", "-d", choices=["MTD", "FTD"], required=True, help="Target device type.")
    parser.add_argument("-v", "--verbose", action="count", default=0, help="Verbosity level (-v for epoch crashes, -vv for signature mappings & non-crashing injections).")
    args = parser.parse_args()

    mapping = {"MTD": ["V1", "V3", "V4"], "FTD": ["V1", "V3", "V5"]}
    active = [c for c in CRASH_SIGNATURES if c.name in mapping[args.device]]
    
    epochs, broken_epochs = parse_log_file(args.logfile)
    analyze_crashes(epochs, active, args.device, broken_epochs, args.verbose)

if __name__ == "__main__":
    main()