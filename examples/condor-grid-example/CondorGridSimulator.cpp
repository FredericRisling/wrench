/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench.h>

#include "CondorWMS.h" // WMS implementation
#include "CondorTimestamp.h"


int main(int argc, char **argv) {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    simulation->init(&argc, argv);

    // Pull platform from provided xml file.
    char *platform_file = argv[1];
    simulation->instantiatePlatform(platform_file);
    double pre_execution_overhead = std::stod(std::string(argv[2]));
    double post_execution_overhead = std::stod(std::string(argv[3]));



    wrench::WorkflowFile *input_file;
    wrench::WorkflowTask *task1;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;

    // Get a hostname
    std::string hostname = "DualCoreHost";

    wrench::Workflow *grid_workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr1;
    workflow_unique_ptr1 = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    grid_workflow = workflow_unique_ptr1.get();

    input_file = grid_workflow->addFile("input_file", 10.0);
    //task1 = grid_workflow->addTask("grid_task1", 87867450000.0, 1, 1, 0);
    //task1 = grid_workflow->addTask("grid_task1", 3050000000.0, 1, 1, 0);
    task1 = grid_workflow->addTask("grid_task1", 753350000000.0, 1, 1, 0);
    task1->addInputFile(input_file);




    // Create a Storage Service
    storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"}));

    //storage_service2 = simulation->add(
    //       new wrench::SimpleStorageService(batchhostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;
    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    auto batch_service = new wrench::BatchComputeService("BatchHost1",
                                                         {"BatchHost1", "BatchHost2"},
                                                         "/scratch",
                                                         {
                                                                 {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                                 {wrench::BatchComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, std::to_string(pre_execution_overhead)},
                                                                 {wrench::BatchComputeServiceProperty::GRID_POST_EXECUTION_DELAY, std::to_string(post_execution_overhead)},
                                                         });


    // Create a HTCondor Service
    compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                    },
                    {},
                    batch_service));

    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMSROBL
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(
            new wrench::CondorWMS({compute_service}, {storage_service}, hostname));

    wms->addWorkflow(grid_workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task" simulation
    simulation->launch();

    simulation->getOutput().dumpUnifiedJSON(grid_workflow, "/tmp/workflow_data.json", false, true, false, false, false, true, false);
    auto start_timestamps = simulation->getOutput().getTrace<wrench::CondorGridStartTimestamp>();
    auto end_timestamps = simulation->getOutput().getTrace<wrench::CondorGridEndTimestamp>();

    for (const auto &start_timestamp : start_timestamps) {
        std::cerr << "Started: " << start_timestamp->getContent()->getDate() << std::endl;
    }
    for (const auto &end_timestamp : end_timestamps) {
        std::cerr << "Ended: " << end_timestamp->getContent()->getDate() << std::endl;
    }

    return 0;
}