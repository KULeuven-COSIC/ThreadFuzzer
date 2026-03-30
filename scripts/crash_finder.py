import argparse
import re
from dataclasses import dataclass, field
from typing import Set, List, Tuple, Dict

# --- DATA STRUCTURES ---

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
    normal_iterations: int = 0
    real_reboots: int = 0
    start_iteration: int = -1
    end_iteration: int = -1
    packets: List[MutatedPacket] = field(default_factory=list)

# --- CRASH DEFINITIONS ---

CRASH_SIGNATURES = [
    MutatedPacket("Crash 1", "Child ID Response", {Mutation("thread_nwd.tlv.prefix.length", "New value=255")}),
    MutatedPacket("Crash 1", "Data Response", {Mutation("thread_nwd.tlv.prefix.length", "New value=255")}), 
    MutatedPacket("Crash 3", "Child ID Response", {Mutation("thread_nwd.tlv.len", "New value=255")}),
    MutatedPacket("Crash 3", "Data Response", {Mutation("thread_nwd.tlv.len", "New value=255")}),
    MutatedPacket("Crash 4", "Child ID Response", {
        Mutation("mle.tlv.addr16", "New value=65535"),
        Mutation("mle.tlv.source_addr", "New value=65535")
    }),
    MutatedPacket("Crash 5", "Advertisement", {
        Mutation("mle.tlv.leader_data.router_id", "New value=255")
    }),
]

# --- PARSING LOGIC ---

def parse_log_file(filepath: str) -> List[EpochData]:
    epochs: List[EpochData] = []
    current_epoch = EpochData(epoch_id=1)
    
    current_summary = ""
    current_mutations = set()
    current_iteration = -1 

    mutation_pattern = re.compile(r"Fuzzed field\s+([a-zA-Z0-9_.]+).*?(New value=[^;]+)")
    iteration_pattern = re.compile(r"START OF A NEW FUZZING ITERATION\s+(\d+)")

    with open(filepath, "r") as file:
        for line in file:
            if "START OF A NEW FUZZING ITERATION" in line:
                current_epoch.normal_iterations += 1
                match = iteration_pattern.search(line)
                if match:
                    current_iteration = int(match.group(1))
                    
                    if current_epoch.start_iteration == -1:
                        current_epoch.start_iteration = current_iteration
                    current_epoch.end_iteration = current_iteration
                    
            elif "REBOOTS:" in line and not "OLD" in line:
                try:
                    current_epoch.real_reboots = int(line.split("REBOOTS:")[1].strip())
                    epochs.append(current_epoch)
                    current_epoch = EpochData(epoch_id=len(epochs) + 1)
                except (IndexError, ValueError):
                    pass
                
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

        if current_summary:
            current_epoch.packets.append(MutatedPacket(
                name="parsed_input", 
                packet_type=current_summary, 
                mutations=current_mutations,
                iteration=current_iteration
            ))
            
        if current_epoch.normal_iterations > 0 or current_epoch.packets:
            epochs.append(current_epoch)

    return epochs


# --- ANALYSIS LOGIC ---

def analyze_crashes(epochs: List[EpochData], active_signatures: List[MutatedPacket], device_type: str):
    first_occurrence: Dict[str, Tuple[int, int]] = {}
    
    # Track using the filtered signatures list
    crash_counts: Dict[str, int] = {f"{crash.name} [{crash.packet_type}]": 0 for crash in active_signatures}

    total_unexpected_reboots = 0
    unexpected_reboots_per_epoch = {}
    
    epoch_size = max((e.normal_iterations for e in epochs), default=0)

    print(f"\n--- FUZZING ANALYSIS REPORT ({device_type} TARGET) ---")
    print(f"Detected Fixed Epoch Size: {epoch_size} iterations")
    
    # Quick debug print so the user knows exactly what is being tracked
    tracked_names = sorted(list(set([c.name for c in active_signatures])))
    print(f"Monitoring for: {', '.join(tracked_names)}")

    global_packet_index = 0 
    
    for epoch in epochs:
        epoch_crash_count = 0
        
        for packet in epoch.packets:
            global_packet_index += 1
            for crash in active_signatures:
                if crash.packet_type in packet.packet_type and crash.mutations.issubset(packet.mutations):
                    
                    crash_key = f"{crash.name} [{crash.packet_type}]"
                    crash_counts[crash_key] += 1
                    epoch_crash_count += 1
                    
                    if crash_key not in first_occurrence:
                        first_occurrence[crash_key] = (global_packet_index, packet.iteration)

        expected_epoch_reboots = 1 + epoch.normal_iterations + epoch_crash_count
        unexpected_diff = epoch.real_reboots - expected_epoch_reboots
        
        if unexpected_diff > 0:
            iter_range = f"Iterations {epoch.start_iteration}-{epoch.end_iteration}" if epoch.start_iteration != -1 else "No iterations"
            
            unexpected_reboots_per_epoch[epoch.epoch_id] = {
                "range": iter_range,
                "unexpected": unexpected_diff,
                "real": epoch.real_reboots,
                "expected": expected_epoch_reboots,
                "iterations": epoch.normal_iterations,
                "crashes": epoch_crash_count
            }
            total_unexpected_reboots += unexpected_diff

    # 1. First occurrence of each crash
    print("\n1. FIRST OCCURRENCE OF EACH CRASH CATEGORY:")
    if not first_occurrence:
        print("   No known crashes detected.")
    else:
        sorted_firsts = sorted(first_occurrence.items(), key=lambda item: item[0])
        for crash_key, (idx, iteration) in sorted_firsts:
            print(f"   - {crash_key}: Packet #{idx} (Iteration {iteration})")

    # 2. Total number of crashes per category
    print("\n2. TOTAL CRASHES PER CATEGORY:")
    sorted_counts = sorted(crash_counts.items(), key=lambda item: item[0])
    for crash_key, count in sorted_counts:
        print(f"   - {crash_key}: {count}")

    # 3. Unexpected Reboots Per Epoch
    print("\n3. UNEXPECTED REBOOTS PER EPOCH:")
    if not unexpected_reboots_per_epoch:
        print("   None! All reboot counts matched expected values.")
    else:
        for epoch_id, stats in unexpected_reboots_per_epoch.items():
            print(f"   - Epoch {epoch_id} ({stats['range']}): {stats['unexpected']} unexpected reboots "
                  f"(Real: {stats['real']}, Expected: {stats['expected']} "
                  f"[1 base + {stats['iterations']} iterations + {stats['crashes']} crashes])")
        
        print(f"\n   => TOTAL UNEXPECTED REBOOTS: {total_unexpected_reboots}")
    print("\n-------------------------------\n")


def main():
    parser = argparse.ArgumentParser(description="Extract fuzzing crash data per epoch from log files.")
    parser.add_argument("logfile", help="Path to the fuzzer log file to analyze.")
    # Add the required device flag
    parser.add_argument("--device", "-d", choices=["MTD", "FTD"], required=True, 
                        help="Target device type (MTD or FTD). Dictates which crashes are expected.")
    args = parser.parse_args()

    # Define the mapping of device types to their expected crash names
    device_expected_crashes = {
        "MTD": ["Crash 1", "Crash 3", "Crash 4"],
        "FTD": ["Crash 1", "Crash 3", "Crash 5"]
    }

    # Filter the global signatures down to only what this device type supports
    expected_names = device_expected_crashes[args.device]
    active_signatures = [crash for crash in CRASH_SIGNATURES if crash.name in expected_names]

    print(f"Parsing log file: {args.logfile} for {args.device} device...")
    
    epochs = parse_log_file(args.logfile)
    total_packets = sum(len(e.packets) for e in epochs)
    
    print(f"Successfully extracted {total_packets} mutated packets across {len(epochs)} epochs.")
    
    # Pass the filtered signatures into the analyzer
    analyze_crashes(epochs, active_signatures, args.device)

if __name__ == "__main__":
    main()