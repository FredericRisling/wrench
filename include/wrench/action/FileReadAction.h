/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_READ_ACTION_H
#define WRENCH_FILE_READ_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class WorkflowFile;
    class FileLocation;

    class FileReadAction : public Action {

    public:
        std::shared_ptr<WorkflowFile> getFile() const;
        std::shared_ptr<FileLocation> getFileLocation() const;

    protected:
        friend class CompoundJob;

        FileReadAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                std::shared_ptr<WorkflowFile> file, std::shared_ptr<FileLocation> file_location);

    private:
        std::shared_ptr<WorkflowFile> file;
        std::shared_ptr<FileLocation> file_location;

    };
}

#endif //WRENCH_FILE_READ_ACTION_H
