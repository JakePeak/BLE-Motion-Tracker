import MakeGraph

graph, axis = MakeGraph.MakeGraph('data3D.csv', 100)

print(MakeGraph.CheckExists(graph))

MakeGraph.Redraw(.25, axis, 'data3D.csv', graph)

ret = str(input("Close Graph: (y/n)"))
if ret == "y":
    MakeGraph.DeleteGraph()

print(MakeGraph.CheckExists(graph))
