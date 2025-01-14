/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/simple/SimpleStorageService.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/ComputeServiceProperty.h"
#include "wrench/services/compute/ComputeServiceMessagePayload.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/workflow/failure_causes/NetworkError.h"

WRENCH_LOG_CATEGORY(wrench_core_compute_service, "Log category for Compute Service");

namespace wrench {


    constexpr unsigned long ComputeService::ALL_CORES;
    constexpr double ComputeService::ALL_RAM;

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void ComputeService::stop() {
        Service::stop();
    }

    /**
     * @brief Submit a job to the compute service
     * @param job: the job
     * @param service_specific_args: arguments specific to compute services when needed:
     *      - to a bare_metal: {}
     *          - If no entry is provided for a taskID, the service will pick on which host and with how many cores to run the task
     *          - If a number of cores is provided (e.g., {"task1", "12"}), the service will pick the host on which to run the task
     *          - If a hostname and a number of cores is provided (e.g., {"task1", "host1:12"}, the service will run the task on that host
     *            with the specified number of cores
     *      - to a BatchComputeService: {"-t":"<int>","-N":"<int>","-c":"<int>"[,{"-u":"<string>"}]} (SLURM-like)
     *         - "-t": number of requested job duration in minutes
     *         - "-N": number of requested compute hosts
     *         - "-c": number of requested cores per compute host
     *         - "-u": username (optional)
     *      - to a CloudComputeService: {}
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void ComputeService::submitJob(std::shared_ptr<WorkflowJob> job, const std::map<std::string, std::string> &service_specific_args) {

        if (job == nullptr) {
            throw std::invalid_argument("ComputeService::submitJob(): invalid argument");
        }

        assertServiceIsUp();

        try {
            if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
                this->submitStandardJob(sjob, service_specific_args);
            } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                this->submitPilotJob(pjob, service_specific_args);
            }
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Terminate a previously-submitted job (which may or may not be running yet)
     *
     * @param job: the job to terminate
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void ComputeService::terminateJob(std::shared_ptr<WorkflowJob> job) {

        if (job == nullptr) {
            throw std::invalid_argument("ComputeService::terminateJob(): invalid argument");
        }

        assertServiceIsUp();

        try {
            if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
                this->terminateStandardJob(sjob);
            } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                this->terminatePilotJob(pjob);
            }
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the compute service runs
     * @param service_name: the name of the compute service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param scratch_space_mount_point: the service's scratch space's mount point ("" if none)
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string service_name,
                                   const std::string mailbox_name_prefix,
                                   std::string scratch_space_mount_point) :
            Service(hostname, service_name, mailbox_name_prefix) {

        this->state = ComputeService::UP;

        if (not scratch_space_mount_point.empty()) {

            try {
                this->scratch_space_storage_service =
                        std::shared_ptr<StorageService>(new SimpleStorageService(hostname, {scratch_space_mount_point}));
                this->scratch_space_storage_service->setScratch();
                this->scratch_space_storage_service_shared_ptr = std::shared_ptr<StorageService>(
                        this->scratch_space_storage_service);
            } catch (std::runtime_error &e) {
                throw;
            }
        } else {
            this->scratch_space_storage_service = nullptr;
        }
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the compute service runs
     * @param service_name: the name of the compute service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param scratch_space: scratch storage space of the compute service (nullptr if none)
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string service_name,
                                   const std::string mailbox_name_prefix,
                                   std::shared_ptr<StorageService> scratch_space) :
            Service(hostname, service_name, mailbox_name_prefix) {

        this->state = ComputeService::UP;
        this->scratch_space_storage_service = scratch_space;
    }

    /**
     * @brief Get whether the compute service supports standard jobs or not
     * @return true or false
     */
    bool ComputeService::supportsStandardJobs() {
        return getPropertyValueAsBoolean(ComputeServiceProperty::SUPPORTS_STANDARD_JOBS);
    }

    /**
     * @brief Get whether the compute service supports pilot jobs or not
     * @return true or false
     */
    bool ComputeService::supportsPilotJobs() {
        return getPropertyValueAsBoolean(ComputeServiceProperty::SUPPORTS_PILOT_JOBS);
    }

    /**
     * @brief Get the number of hosts that the compute service manages
     * @return the host count
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long ComputeService::getNumHosts() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        unsigned long count = 0;
        if (dict.find("num_hosts") != dict.end()) {
            count += (unsigned long) (*(dict["num_hosts"].begin())).second;
        }
        return count;
    }

    /**
      * @brief Get the list of the compute service's compute host
      * @return a vector of hostnames
      *
      * @throw WorkflowExecutionException
      * @throw std::runtime_error
      */
    std::vector<std::string> ComputeService::getHosts() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::vector<std::string> to_return;

        if (dict.find("num_cores") != dict.end()) {
            for (auto x : dict["num_cores"]) {
                to_return.emplace_back(x.first);
            }
        }

        return to_return;
    }



    /**
      * @brief Get core counts for each of the compute service's host
      * @return a map of core counts, indexed by hostnames
      *
      * @throw WorkflowExecutionException
      * @throw std::runtime_error
      */
    std::map<std::string, unsigned long> ComputeService::getPerHostNumCores() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::map<std::string, unsigned long> to_return;

        if (dict.find("num_cores") != dict.end()) {
            for (auto x : dict["num_cores"]) {
                to_return.insert(std::make_pair(x.first, (unsigned long) x.second));
            }
        }

        return to_return;
    }

    /**
      * @brief Get the total core counts for all hosts of the compute service
      * @return total core counts
      *
      * @throw WorkflowExecutionException
      * @throw std::runtime_error
      */
    unsigned long ComputeService::getTotalNumCores() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        unsigned long count = 0;
        if (dict.find("num_cores") != dict.end()) {
            for (auto x : dict["num_cores"]) {
                count += (unsigned long) x.second;
            }
        }
        return count;
    }


    /**
     * @brief Get idle core counts for each of the compute service's host
     * @return the idle core counts (could be empty)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::map<std::string, unsigned long> ComputeService::getPerHostNumIdleCores() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::map<std::string, unsigned long> to_return;

        if (dict.find("num_idle_cores") != dict.end()) {
            for (auto x : dict["num_idle_cores"]) {
                to_return.insert(std::make_pair(x.first, (unsigned long) x.second));
            }
        }

        return to_return;
    }

    /**
     * @brief Get ram availability for each of the compute service's host
     * @return the ram availability map (could be empty)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::map<std::string, double> ComputeService::getPerHostAvailableMemoryCapacity() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::map<std::string, double> to_return;

        if (dict.find("ram_availabilities") != dict.end()) {
            for (auto x : dict["ram_availabilities"]) {
                to_return.insert(std::make_pair(x.first, (double) x.second));
            }
        }

        return to_return;
    }

    /**
     * @brief Get the total idle core count for all hosts of the compute service
     * @return total idle core count
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long ComputeService::getTotalNumIdleCores() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }


        unsigned long count = 0;
        if (dict.find("num_cores") != dict.end()) {
            for (auto x : dict["num_idle_cores"]) {
                count += (unsigned long) x.second;
            }
        }
        return count;
    }

    /**
     * @brief Method to find out if, right now, the compute service has at least one host
     *        with some idle number of cores and some available RAM
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     * @return true if idle resources are available, false otherwise
     */
    bool ComputeService::isThereAtLeastOneHostWithIdleResources(unsigned long num_cores, double ram) {
        assertServiceIsUp();

        // send a "info request" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_service_info");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(
                    answer_mailbox,
                    num_cores,
                    ram,
                    this->getMessagePayloadValue(
                            ComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the reply
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage*>(message.get())) {
            return msg->answer;

        } else {
            throw std::runtime_error(
                    "bare_metal::isThereAtLeastOneHostWithIdleResources(): unexpected [" + msg->getName() +
                    "] message");
        }
    }

    /**
    * @brief Get the per-core flop rate of the compute service's hosts
    * @return a list of flop rates in flop/sec
    *
    * @throw WorkflowExecutionException
    */
    std::map<std::string, double> ComputeService::getCoreFlopRate() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::map<std::string, double> to_return;
        if (dict.find("flop_rates") != dict.end()) {
            for (auto x : dict["flop_rates"]) {
                to_return.insert(std::make_pair(x.first, x.second));
            }
        }

        return to_return;
    }

    /**
    * @brief Get the RAM capacities for each of the compute service's hosts
    * @return a map of RAM capacities, indexed by hostname
    *
    * @throw WorkflowExecutionException
    */
    std::map<std::string, double> ComputeService::getMemoryCapacity() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        std::map<std::string, double> to_return;

        if (dict.find("ram_capacities") != dict.end()) {
            for (auto x : dict["ram_capacities"]) {
                to_return.insert(std::make_pair(x.first, x.second));
            }
        }

        return to_return;
    }

    /**
     * @brief Get the time-to-live of the compute service
     * @return the ttl in seconds
     *
     * @throw WorkflowExecutionException
     */
    double ComputeService::getTTL() {

        std::map<std::string, std::map<std::string, double>> dict;
        try {
            dict = this->getServiceResourceInformation();
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        return (*(dict["ttl"].begin())).second;
    }

    /**
     * @brief Get information about the compute service as a dictionary of vectors
     * @return service information
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::map<std::string, std::map<std::string, double>> ComputeService::getServiceResourceInformation() {

        assertServiceIsUp();

        // send a "info request" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_service_info");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ComputeServiceResourceInformationRequestMessage(
                    answer_mailbox,
                    this->getMessagePayloadValue(
                            ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the reply
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceResourceInformationAnswerMessage*>(message.get())) {
            return msg->info;

        } else {
            throw std::runtime_error(
                    "bare_metal::getServiceResourceInformation(): unexpected [" + msg->getName() +
                    "] message");
        }
    }

    /**
     * @brief Get the total capacity of the compute service's scratch storage space
     * @return a size (in bytes)
     */
    double ComputeService::getTotalScratchSpaceSize() {
        // A scratch space SS is always created with a single mount point
        return this->scratch_space_storage_service ? this->scratch_space_storage_service->getTotalSpace().begin()->second : 0.0;
    }

    /**
     * @brief Get the free space on the compute service's scratch storage space
     * @return a size (in bytes)
     */
    double ComputeService::getFreeScratchSpaceSize() {
        // A scratch space SS is always created with a single mount point
        return this->scratch_space_storage_service ? this->scratch_space_storage_service->getFreeSpace().begin()->second : 0.0;
    }

    /**
    * @brief Get the compute service's scratch storage space
    * @return a pointer to the shared scratch space
    */
    std::shared_ptr<StorageService> ComputeService::getScratch() {
        return this->scratch_space_storage_service;
    }

    /**
    * @brief Get a shared pointer to the compute service's scratch storage space
    * @return a shared pointer to the shared scratch space
    */
    std::shared_ptr<StorageService> ComputeService::getScratchSharedPtr() {
        return this->scratch_space_storage_service_shared_ptr;
    }

    /**
    * @brief Checks if the compute service has a scratch space
    * @return true if the compute service has some scratch storage space, false otherwise
    */
    bool ComputeService::hasScratch() {
        return (this->scratch_space_storage_service != nullptr);
    }

};
