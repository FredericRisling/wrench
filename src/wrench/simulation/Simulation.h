/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>
#include <compute_services/multicore_job_executor/MulticoreJobExecutor.h>

#include "simgrid_S4U_util/S4U_Simulation.h"


#include "workflow/Workflow.h"
#include "wms/WMS.h"


namespace wrench {

    /**
     * @brief A class that keeps track of
    *  the simulation state.
     */
    class Simulation {

    public:
        Simulation();

        ~Simulation();

        void init(int *, char **);

        void instantiatePlatform(std::string);

        std::set<std::string> getHostnameList();

        void launch();

        void add(std::unique_ptr<MulticoreJobExecutor> executor);

        void createWMS(std::string, std::string, Workflow *, std::string);

        void add_static_optimization(StaticOptimization *);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void shutdownAllComputeServices();

        std::set<ComputeService *> getComputeServices();

        /***********************/
        /** \endcond            */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        static wrench::MulticoreJobExecutor *createUnregisteredMulticoreJobExecutor(
                std::string, bool, bool, std::map<MulticoreJobExecutor::Property, std::string> plist,
                unsigned int num_cores,
                double ttl, PilotJob *pj, std::string suffix);

        void mark_compute_service_as_terminated(ComputeService *cs);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        std::unique_ptr<S4U_Simulation> s4u_simulation;
        std::unique_ptr<WMS> wms;

        std::vector<std::unique_ptr<ComputeService>> running_compute_services;
        std::vector<std::unique_ptr<ComputeService>> terminated_compute_services;

    };

};

#endif //WRENCH_SIMULATION_H
