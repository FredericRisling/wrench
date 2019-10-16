function initialise() {
    if (energyData != []) {
        generateConsumedEnergyGraph(energyData, "consumedEnergyGraph", "consumedEnergyGraphLegend")
        generatePStateGraph(energyData, "pStateGraph", "pStateGraphLegend")
    }
    var noFileDiv = document.getElementById("no-file")
    var mainBodyDiv = document.getElementById("main-body")
    if (localStorage.getItem("firstVisit") === "null") {
        localStorage.setItem("firstVisit", true)
        firstVisit = true
    }
    if (data.file === undefined) {
        noFileDiv.style.display = "block"
        mainBodyDiv.style.display = "none"
    } else {
        noFileDiv.style.display = "none"
        mainBodyDiv.style.display = "block"
        generateGraph(data.contents, "graph-container", currGraphState)
        populateLegend("taskView")
        populateWorkflowTaskDataTable(data.contents, "task-details-table", "task-details-table-body", "task-details-table-td")
        getOverallWorkflowMetrics(data.contents, "overall-metrics-table", "task-details-table-td")
        generate3dGraph(data.contents, true, "three-d-graph-svg", "origin-x", "origin-y", "time-interval", "scale-input")
    }
}

initialise()