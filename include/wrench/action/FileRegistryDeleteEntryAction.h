/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_REGISTRY_DELETE_ENTRY_ACTION_H
#define WRENCH_FILE_REGISTRY_DELETE_ENTRY_ACTION_H

#include <string>

#include "wrench/action/FileRegistryAction.h"

namespace wrench {

    class WorkflowFile;
    class FileLocation;
    class FileRegistryService;

    class FileRegistryDeleteEntryAction : public FileRegistryAction {

    public:

    protected:
        friend class CompoundJob;

        FileRegistryDeleteEntryAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                std::shared_ptr<FileRegistryService> file_registry_service,
                                WorkflowFile *file,
                                std::shared_ptr<FileLocation> file_location) :
                                FileRegistryAction(FileRegistryAction::DELETE, name, std::move(job), std::move(file_registry_service), file, std::move(file_location)) {}

//        void execute(std::shared_ptr<ActionExecutor> action_executor,unsigned long num_threads, double ram_footprint) override;
//        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;

    private:
        std::shared_ptr<FileRegistryService> file_registry_service;
        WorkflowFile *file;
        std::shared_ptr<FileLocation> file_location;

    };
}

#endif //WRENCH_FILE_REGISTRY_DELETE_ENTRY_ACTION_H