	request_handlers["advanceTime"] = [sc](json data) { return sc->advanceTime(std::move(data)); };
	request_handlers["getTime"] = [sc](json data) { return sc->getSimulationTime(std::move(data)); };
	request_handlers["waitForNextSimulationEvent"] = [sc](json data) { return sc->waitForNextSimulationEvent(std::move(data)); };
	request_handlers["getSimulationEvents"] = [sc](json data) { return sc->getSimulationEvents(std::move(data)); };
	request_handlers["getAllHostnames"] = [sc](json data) { return sc->getAllHostnames(std::move(data)); };
	request_handlers["standardJobGetTasks"] = [sc](json data) { return sc->getStandardJobTasks(std::move(data)); };
	request_handlers["addBareMetalComputeService"] = [sc](json data) { return sc->addBareMetalComputeService(std::move(data)); };
	request_handlers["createStandardJob"] = [sc](json data) { return sc->createStandardJob(std::move(data)); };
	request_handlers["submitStandardJob"] = [sc](json data) { return sc->submitStandardJob(std::move(data)); };
	request_handlers["createTask"] = [sc](json data) { return sc->createTask(std::move(data)); };
	request_handlers["taskGetFlops"] = [sc](json data) { return sc->getTaskFlops(std::move(data)); };
	request_handlers["taskGetMinNumCores"] = [sc](json data) { return sc->getTaskMinNumCores(std::move(data)); };
	request_handlers["taskGetMaxNumCores"] = [sc](json data) { return sc->getTaskMaxNumCores(std::move(data)); };
	request_handlers["taskGetMemory"] = [sc](json data) { return sc->getTaskMemory(std::move(data)); };