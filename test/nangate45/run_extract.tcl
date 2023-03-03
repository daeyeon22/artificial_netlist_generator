read_lef ./tech_lef/NangateOpenCellLibrary_revised.lef
read_def ./bench/circaug_0/def/circaug_0_0.7_1.0_7.def
# AVG_TOPO_ORDER:   average topological order of instances in reg2reg / in2reg / reg2out 
#                   this tool estimates the synthesis clock period of top module as avg_topo_order * avg_gate_delay [ns]
# AVG_GATE_DELAY:   average gate delay for given target technology (ns)
#                   we will delete this input arg, but it is required at this version....
#                   just give an estimated value 
# 
# optional (default : 0.1 [ns])
#artnetgen_set_parameter -avg_gate_delay 1.0
artnetgen_write_spec -out_file ./spec_files_from_def/circaug_0.spec
exit

