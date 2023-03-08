# Artificial Netlist Generator

## Introduction
The structural characteristics of a place and route (P&R) benchmark vary widly depending on the logic and synthesis method with target technologies. The structural characteristics affect the solution space of CAD algorithms (e.g., partitioning, placement, routing, etc), so insufficient number of P&R benchmarks may lead to the poor generalizability for unseen circuits. 

Research communities have tried to publish open-source benchmarks to support CAD research in recent decades, but they are mapped to a specific technology node or written in a data format that is not supported by commercial tools. Furthermore, to protect the intellectual property of the developers, the data must be anonymized (i.e., technical details of the circuits). 

Researchers somtimes create several new versions of circuits by combining multiple modules or modifying functionality at the RTL level, but these modifications cannot provide a large variation in circuit characteristics compared to the original circuit and consume a lot of human resources for data scalability.

One of the solutions to mitigate the lack of circuit benchmarks is to generate artificial circuits that represent realistic circuit characteristics. Artificial circuits can be generated using specific rules and programs so that it is cost-less compared to real-world data in terms of data scalability. 
This is a strong motivartion why we developed this program called 'artificla netlist generator' (ANG).

ANG can create any artificial gate-level netlist from user-specified parameters representing the topological characteristics of the circuit. ANG takes six topological parameters as inputs

---
**Topological parameters**
|Flag|Description|Recommended value|
|------|---|---|
|-num_insts|number of instances||
|-num_primary_ios|number of primary input and outputs|upto 10% of the number of instances|
|-avg_net_degree|average net degree(=average number of fanouts)|2.5 ~ 4.0|
|-avg_net_bbox|average size of net bounding box (=average hpwl after placement)|0.1 ~ 2.0|
|-avg_topo_order|average depth of timing paths (= average depth of leaf nodes)|5.0 ~ 15.0|
|-comb_ratio|ratio of combinational logic gates over the number of instances|0.75 ~ 0.95|


---

## Usage

    ./openroad
    read_lef <lef path>
    artnetgen_create_spec -num_insts <number of instances>
                          -num_primary_ios <number of primary input and output>
                          -comb_ratio <ratio of combinational gates>
                          -avg_bbox <average size of net bounding box>
                          -avg_net_degrere <average net degree>
                          -avg_topo_order <average depth of timing paths>
                          -cell_list <cell list to use>
                          -out_file <spec path>
              
    
    generate_artificial_netlist -top_module <top module name>
                                -spec_file <spec path>
                                
    artnetgen_write_verilog -out_file <verilog path>
    artnetgen_write_sdc -out_file <sdc path>


## How to build

    git clone --recursive https://github.com/daeyeon22/artificial_netlist_generator.git
  
    cd artificial_netlist_generator

    mkdir build & cd build & cmake ..
  
    make -j

## Features
Construction of Realistic Place-and-route Benchmarks for Machine Learning Applications [(link)](https://ieeexplore.ieee.org/document/9904912)
This code is based on OpenROAD project [(link)](https://github.com/The-OpenROAD-Project/OpenROAD)

# Submodule List

# Authors
