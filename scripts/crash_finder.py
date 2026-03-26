import os
import glob
from dataclasses import dataclass

# EPOCH_SIZE = (
#     2  # includes the iteration check at the end (in this case a single iteration)
# )
lines = []


# NOTE: example, from an older version
# with open("/home/jakob/Documents/uni/doc/project/COTS_TESTS/ThreadFuzzer/EVE_SENSOR/run_2025-11-18_15-47-12/fuzzer_logs/info/threadfuzzer_info_2025-11-18_15:47:12.log", "r") as f:
#    lines = f.readlines()

workdir = os.path.join(os.getcwd(), "../logs/_data/EVE_SENSOR")
print("workdir:", workdir)
idx = 25  # should be 23, 25 and 10
file_names = sorted(glob.glob(os.path.join(workdir, "*")))
print("file_names:", file_names)
for i, f in enumerate(file_names):
    if i == idx:
        print(i, "->", f)
    else:
        print(i, f)
info_file = glob.glob(os.path.join(file_names[idx], "fuzzer_logs/info/*"))[0]
print(info_file)


# info_file  = "/home/jakob/Documents/uni/doc/project/COTS_TESTS/ThreadFuzzer/EVE_SENSOR/run_2025-11-18_15-47-12/fuzzer_logs/info/threadfuzzer_info_2025-11-18_15:47:12.log"


# use of generators to quickly parse big files
def read_lines_gen(fname):
    with open(fname, "r") as file:
        for line in file:
            # if "Dissector wpan" in line:
            #     continue
            # if "Fuzzed packet" in line:
            #     continue
            # if "<---" in line:
            #     continue
            if (
                "---> Dissector" in line
                or "Mutator:" in line
                or "ITERATION" in line
                or "REBOOTS:" in line
            ):
                yield line


# with open(info_file) as f:
#     lines = f.readlines()

markers = {}


def packet_extractor():
    summary = ""
    mutators = []
    packets = []
    for i, line in enumerate(read_lines_gen(info_file)):
        # print(line)
        if "ITERATION" in line:
            print("current_size:", len(packets))
            markers[len(packets)] = line
            print((len(packets), line))
        elif "REBOOTS:" in line:
            print("current_size:", len(packets))
            markers[len(packets)] = line
            print((len(packets), line))
        elif "---> Dissector's summary" in line:
            if summary == "":
                summary = line.split("summary: ")[1]
            else:
                packets.append((summary, mutators))
                # print("extracted:",(summary,mutators))
                summary = line.split("summary: ")[1]
                mutators = []
        elif "Mutator" in line and summary != "":
            mutators.append(line)
    if len(mutators) != 0 or summary != "":
        # print("orphaned_muts:", mutators, "or")
        # print("orphaned_summ:", summary)
        if summary != "":
            packets.append((summary, mutators))
    return packets


@dataclass(frozen=True)
class Mutation:
    field: str
    new_val: str


@dataclass
class MutatedPacket:
    name: str
    packet_type: str
    mutations: {Mutation}


def crash_matcher(crash, packet):
    if crash.packet_type in packet.packet_type and crash.mutations.issubset(
        packet.mutations
    ):
        print("Crash:", crash.name)
        return True
    return False


# TODO: also create versions, for the different packet types!
# like c1 can also be written for Data Response
c1 = MutatedPacket(
    "crash 1",
    "Child ID Response",
    set([Mutation("thread_nwd.tlv.prefix.length", "New value=255")]),
)

c1_dr = MutatedPacket(
    "crash 1, data response",
    "Data Response",
    set([Mutation("thread_nwd.tlv.prefix.length", "New value=255")]),
)

c2 = MutatedPacket(
    "crash 2",
    "Child ID Response",
    set(
        [
            Mutation("thread_nwd.tlv.len", "New value=0"),
            Mutation("thread_nwd.tlv.server.16", "New value=40961"),
        ]
    ),
)

c3 = MutatedPacket(
    "crash 3",
    "Child ID Response",
    set([Mutation("thread_nwd.tlv.len", "New value=255")]),
)

c3_dr = MutatedPacket(
    "crash 3, data response",
    "Data Response",
    set([Mutation("thread_nwd.tlv.len", "New value=255")]),
)

# c4 = MutatedPacket(
#     "crash 4",
#     "Child Update Response",
#     set([Mutation("mle.tlv.timeout", "New value=4294967295")]),
# )

c5 = MutatedPacket(
    "crash 5",
    "Advertisement",
    set([Mutation("mle.tlv.leader_data.router_id", "New value=255")]),
)


c9 = MutatedPacket(
    "crash 9",
    "Child ID Response",
    set(
        [
            Mutation("mle.tlv.addr16", "New value=65535"),
            Mutation("mle.tlv.source_addr", "New value=65535"),
        ]
    ),
)

# TODO read in the crashes from JSON, more flexible
crash_muts = [
    c1,
    c1_dr,
    # c2, # impossible on an MTD
    c3,
    c3_dr,
    # c5, # impossible on an MTD
    c9,   # extremely rare
]


cnt = 0
cnt_max = 1000
mult = cnt_max // 100
# print("[", end='')
# gen = packet_extractor()
# packet = next(gen)
# while packet is not None:
#     packets.append(packet)
#     packet = next(gen)
#     # print(packet)
#     cnt += 1
#     if (cnt % mult) == 0:
#         print(cnt)
#     if cnt > cnt_max:
#         break

packets = packet_extractor()
# print("]")

print("collected:", len(packets), "packets")
# print("leaving:", len(line_buffer), "lines unparsed...")
print("")
print("...start analysis...")
print("")
cid_cnt = 0
for i, p in enumerate(packets):
    # turn into MutatedPacket element
    raw_muts = p[1]
    muts = []
    for rm in raw_muts:
        try:
            fname = rm.split("Mutator:")[1].split("Fuzzed field ")[1].split(" ")[0]
            nval = rm.split("Mutator:")[1].split("Fuzzed field ")[1].split(";")[1][1:]
            muts.append(Mutation(fname, nval))
        except IndexError:
            continue
        # if 'Child ID Response' in p[0] and 'thread_nwd.tlv.prefix.length' in rm:
        #     if 'New value=255' in rm:
        #         print("WE SHOULD FIND THIS:", i, p, muts[len(muts)-1], sep='\n')
    if len(muts) != 0:
        # NOTE to check
        # NOTE end check
        # if i == 4888:
        #     print("----------------")
        #     print(c1.packet_type)
        #     print(p[0])
        #     print(c1.mutations)
        #     print(set(muts))
        if "Child ID Response" in p[0]:
            cid_cnt += 1
        mut_packet = MutatedPacket("input", p[0], set(muts))
        # print(mut_packet)
        for crash in crash_muts:
            # crash_matcher(crash, mut_packet)
            if crash_matcher(crash, mut_packet):
                print("at:", i)
        # crash_matcher(c9, MutatedPacket("input2", p[0], set(muts)))
        # if c1.packet_type in p[0] and c1.mutations.issubset(set(muts)):
        #     print("Crash_1:", i)
        # if c9.packet_type in p[0] and c9.mutations.issubset(set(muts)):
        #     print("Crash_9:", i)

    # print possible markers:
    if i in markers.keys():
        print("")
        print(i, markers[i])


print("cidcnt:", cid_cnt)
