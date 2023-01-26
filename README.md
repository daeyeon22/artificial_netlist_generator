# Artificial Netlist Generator

## Introduction
The structural characteristics of a place and route (P&R) benchmark vary widly depending on the logic and synthesis method with target technologies. The structural characteristics affect the solution space of CAD algorithms (e.g., partitioning, placement, routing, etc), so insufficient number of P&R benchmarks may lead to the poor generalizability for unseen circuits. 

Research communities have tried to publish open-source benchmarks to support CAD research in recent decades, but they are mapped to a specific technology node or written in a data format that is not supported by commercial tools. Furthermore, to protect the intellectual property of the developers, the data must be anonymized (i.e., technical details of the circuits). 

Researchers somtimes create several new versions of circuits by combining multiple modules or modifying functionality at the RTL level, but these modifications cannot provide a large variation in circuit characteristics compared to the original circuit and consume a lot of human resources for data scalability.

One of the solutions to mitigate the lack of circuit benchmarks is to generate artificial circuits that represent realistic circuit characteristics. Artificial circuits can be generated using specific rules and programs so that it is cost-less compared to real-world data in terms of data scalability. 
This is a strong motivartion why we developed this program called 'artificla netlist generator' (ANG).

ANG can create any artificial gate-level netlist from user-specified parameters representing the topological characteristics of the circuit. ANG takes six topological parameters as inputs: (1) number of instances, (2) number of primary input and outputs, (3) average net degree (=average number of fanouts), (4) average size of net bounding box (=average hpwl after placement), (5) average depth of timing paths (= average depth of leaf nodes), (6) ratio of sequential logic gates.

## Usage

## How to build


## Features
Construction of Realistic Place-and-route Benchmarks for Machine Learning Applications [(link)](https://ieeexplore.ieee.org/document/9904912)
This code is based on OpenROAD project [(link)](https://github.com/The-OpenROAD-Project/OpenROAD)

# Submodule List

# Authors
