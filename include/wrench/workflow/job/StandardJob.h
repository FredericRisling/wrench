/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STANDARDJOB_H
#define WRENCH_STANDARDJOB_H


#include <map>
#include <set>
#include <vector>

#include "WorkflowJob.h"

#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class WorkflowFile;

    class WorkflowTask;

    /**
     * @brief A standard (i.e., non-pilot) workflow job that can be submitted to a ComputeService
     * by a WMS (via a JobManager)
     */
    class StandardJob : public WorkflowJob {

    public:
        /** @brief Standad job states */
        enum State {
            /** @brief Not submitted yet */
                    NOT_SUBMITTED,
            /** @brief Submitted but not running yet */
                    PENDING,
            /** @brief Running */
                    RUNNING,
            /** @brief Completed successfully */
                    COMPLETED,
            /** @brief Failed */
                    FAILED,
            /** @brief Terminated by submitter */
                    TERMINATED
        };

        std::vector<WorkflowTask *> getTasks();

        unsigned long getMinimumRequiredNumCores();

        unsigned long getMinimumRequiredMemory();

        unsigned long getNumCompletedTasks();

        unsigned long getNumTasks();

        StandardJob::State getState();

        std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> getFileLocations();

        unsigned long getPriority();


        /** @brief The job's computational tasks */
        std::vector<WorkflowTask *> tasks;

        /** @brief The job's total computational cost (in flops) */
        double total_flops;
        /** @brief The number of computational tasks that have completed */
        unsigned long num_completed_tasks;

        /** @brief The file locations that tasks should read/write files from/to. Each file is given a list of locations, in preferred order */
        std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations;

        /** @brief The ordered file copy operations to perform before computational tasks */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>  >> pre_file_copies;
        /** @brief The ordered file copy operations to perform after computational tasks */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>  >> post_file_copies;
        /** @brief The ordered file deletion operations to perform at the end */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions;

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        double getPreJobOverheadInSeconds();
        void setPreJobOverheadInSeconds(double overhead);
        double getPostJobOverheadInSeconds();
        void setPostJobOverheadInSeconds(double overhead);


    private:

        friend class StandardJobExecutor;
        friend class BareMetalComputeService;
        void incrementNumCompletedTasks();

        friend class JobManager;

        StandardJob(Workflow *workflow,
                    std::vector<WorkflowTask *> tasks, std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> &file_locations,
                    std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>>> &pre_file_copies,
                    std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>>> &post_file_copies,
                    std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> &cleanup_file_deletions);

        State state;
        double pre_overhead = 0.0;
        double post_overhead = 0.0;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_MULTITASKJOB_H
