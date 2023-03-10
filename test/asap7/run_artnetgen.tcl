read_lef ./lef/asap7_tech_1x_201209.lef
read_lef ./lef/asap7sc7p5t_28_R_1x_220121a.lef
# NUM_INSTS         :   the number of instances (count only comb/sequ logic gates)
# NUM_PRIMARY_IOS   :   the number of primary inputs and outputs 
# COMB_RATIO        :   the ratio of combinational logic gates over NUM_INSTS
# AVG_NET_DEGREE    :   the average of net degree (~=average fanout)
# AVG_BBOX          :   the average size of net bounding box 
#                       this parameter can be measured after placement
#                       it will be normalized by bin_size 
#                       * bin_size = die_size/bin_count
#                       * bin_count = sqrt(NUM_INSTS)
# CELL_LIST         :   list of standard cell types that you want to use
#                       specify the cell count (refer onlyUseCell.list file)
# SPEC_PATH         :   the path of .spec file to store
# AVG_TOPO_ORDER    :   average topological order of instances in reg2reg / in2reg / reg2out 
#                       this tool estimates the synthesis clock period of top module as avg_topo_order * avg_gate_delay [ns]
# AVG_GATE_DELAY    :   average gate delay for given target technology (ns)
#                       we will delete this input arg, but it is required at this version....
#                       just give an estimated value 



artnetgen_create_spec \
    -num_insts 11591 \
    -num_primary_ios 53 \
    -comb_ratio 0.90 \
    -avg_net_degree 2.18 \
    -avg_bbox 0.82 \
    -avg_topo_order 8.33 \
    -cell_list ./onlyUseCell.list \
    -out_file ./test_circuit.spec


artnetgen_init  -top_module test_circuit \
                -spec_file  test_circuit.spec \
                -verbose 6 

# optional (default : 0.1 [ns])
# artnetgen_set_parameter -avg_gate_delay __AVG_GATE_DELAY__

# set_dont_use clock interter types (example)
artnetgen_set_parameter -dont_use CKINVDCx10_ASAP7_75t_R
artnetgen_set_parameter -dont_use CKINVDCx11_ASAP7_75t_R
artnetgen_set_parameter -dont_use CKINVDCx12_ASAP7_75t_R
artnetgen_set_parameter -dont_use CKINVDCx14_ASAP7_75t_R
artnetgen_set_parameter -dont_use CKINVDCx16_ASAP7_75t_R
artnetgen_set_parameter -dont_use CKINVDCx20_ASAP7_75t_R

# check the list of available masters
artnetgen_print_masters

# if you finished setting parameters, run
artnetgen_run


artnetgen_write_verilog -out_file test_circuit.v 
artnetgen_write_sdc -out_file test_circuit.sdc
exit
