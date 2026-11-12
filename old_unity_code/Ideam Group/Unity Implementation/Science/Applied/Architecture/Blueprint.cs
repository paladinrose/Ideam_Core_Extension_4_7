using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Applied.Architecture;
using Ideam.Science.Formal.Mathematics;

[CreateAssetMenu(menuName ="Ideam/Procedural Generation/Procedural Blueprint")]
public class Blueprint : ScriptableObject
{
    /*
     I've got, I believe, the ability to add new nodes. 
     


Process from tutorial:
Every drawing is about what, specifically, you're trying to communicate.

Goals of a floorplan: 
-Show the relationship of spaces to one another
-Describe all the physical features of each space, interior and exterior
-Precisely record and reflect the real dimensions.
-Serves as a map to show the tradespeople who will actually be building it.
-Typically, a floor plan is presented as a horizontal slice through a level, at about 4 feet in height.
-The elements you're cutting through - primariy the walls -  should have the thickest, heaviest line-weight.
-I need an element to list layers, create new ones, and turn their visibility on and off.
-Let's make a color table that describes the pallet I list down in Layers. Changing a color in the table
changes the blueprint in real time.
-Map Key. Mainly for labels and symbols. Again, I change the symbol, here, it changes all over the floorplan.
- 

Layers: (It's important to remember that things across my layers will be linked together and changing something on one layer, will change linked things on other layers, even if you can't see them)

-Spaces (Rooms, courtyards, porches, etc. Represented as complex polygonal shapes (Imma use Spline2's of course)). Thin line weight. Rooms can each have a color assigned (but I should be sparing and use good color theory. Aim for being easy to read by the color-blind population)
-Walls. Exterior heavier than interior. Heavy line weight, uniform color
-Features. Medium line weight, uniform color
-Labels. Light line weight, uniform color per TYPE of label.
-Measurements. Light line weight, uniform color.

     */


    public Vector2 size;
    public ProceduralBlueprintGraph graph;

}

[Serializable]
public class ProceduralBlueprintGraph : IGraph, IBlueprint
{
    //Master node that creates ports for all the features and connects feature nodes to those ports.
    public ProceduralBlueprintNode blueprintNode;

    public ProceduralBlueprintFeatureNode[] features;

    public int NodesCount { get { return features.Length; } }
    public int EdgesCount { get; }

    public int AddEdge<T>(T edge) where T : IEdge
    {
        throw new System.NotImplementedException();
    }

    public int AddNode<T>(T node) where T : INode
    {
        if(node is ProceduralBlueprintFeatureNode featureNode)
        {
            for(int i =0; i < NodesCount; i++)
            {
                if(features[i] == featureNode) { return i; }
            }


        }
        return -1;
    }

    public T[] GetBuildings<T>() where T : IBuilding
    {
        throw new System.NotImplementedException();
    }

    public T GetEdge<T>(int id) where T : IEdge
    {
        throw new System.NotImplementedException();
    }

    public T[] GetEdges<T>() where T : IEdge
    {
        throw new System.NotImplementedException();
    }

    public T[] GetFeatures<T>() where T : IFeature
    {
        throw new System.NotImplementedException();
    }

    public T GetNode<T>(int id) where T : INode
    {
        throw new System.NotImplementedException();
    }

    public T[] GetNodes<T>() where T : INode
    {
        throw new System.NotImplementedException();
    }

    public int[] GraphSearch(int startCell, int endCell)
    {
        throw new System.NotImplementedException();
    }

    public void RemoveEdge(int localID)
    {
        throw new System.NotImplementedException();
    }

    public void RemoveNode(int localID)
    {
        throw new System.NotImplementedException();
    }
}

[Serializable]
public class ProceduralBlueprintNode : INode
{
    public Float2 position;
    public Float2 Position { get { return position; } }

    public T[] GetPorts<T>() where T : IPort
    {
        return new T[0];
    }

    public void SetPosition(Float2 newPosition)
    {
        position = newPosition;
    }
}

[Serializable]
public class ProceduralBlueprintFeatureNode : INode
{
    public Float2 position;
    public Float2 Position { get { return position; } }

    public T[] GetPorts<T>() where T : IPort
    {
        return new T[0];
    }

    public void SetPosition(Float2 newPosition)
    {
        position = newPosition;
    }
}
