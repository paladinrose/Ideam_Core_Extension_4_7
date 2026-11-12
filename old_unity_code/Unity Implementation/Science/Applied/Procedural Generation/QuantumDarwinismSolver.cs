using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

public class QuantumDarwinismSolver : MonoBehaviour
{

    //This takes control of the Procedural Wallpaper, so that it can run Asset Distribution itself, based on a 
    //set of Pointer States.
    //Now, get my Hospital Map generating with Walls (with or without doors) in the Edge Asset, Floors in the Symmetry asset, and columns or some such for the Point asset.
    //Generate Landmarks first - important parts of the map that MUST be there, like the starting room and the boos room.
    //Next, generate paths between all the landmarks - these will ensure floors are whole, and walls have appropriate doors or openings.
    //Now generate Floors, making sure that if a floor is open and cuts off access to rooms beyond, there's at least one path to every room. No islands, etc.
    //Ok, now we move on to walls. Some will already be set by landmarks and paths. The rest use them, and floors, as Pointer States and solve out from there.
    //Lastly, add Point assets, props, and enemies by using everything else - walls, floors, landmarks - as pointer states.

    public IEnumerator SolveGameBoard2D(UnityGameBoard board, SymmetryGroupType quantizedSymmetryType = SymmetryGroupType.P1)
    {

        //Step 1: Quantize the Board:
        Bounds2 boardBounds = board.Bounds<Bounds2, Float2>();

        Spline2 boundsSpline = new Spline2(new Float2[]
        {
            boardBounds.Min,
            new Float2(boardBounds.Min.x, boardBounds.Max.y),
            boardBounds.Max,
            new Float2(boardBounds.Max.x, boardBounds.Min.y)
        });
        
        ComplexSymmetryGroup quantizedBoard = new ComplexSymmetryGroup();
        IEnumerator quantizationProcess = ComplexSymmetryGroup.WallpaperProcess(quantizedSymmetryType, new Spline2[] { boundsSpline }, boardBounds.Center, 
            finishedField => 
            {
                quantizedBoard = finishedField;
            });

        while (quantizationProcess.MoveNext()) { yield return null; }

        //I now have the bounds of my board, and a quantized board. The board gives me the following properties:
        //  A. The Verts that connect all the Edges.
        //  B. The Edges that surround each symmetry.
        //  C. The Symmetries which are each a quantized space on our board.


        //2. I need further input. Right now, everything is completely unset. There ARE no pointer states. I need at least one starting Pointer State.
        UnityGameBoardSpace[] boardSpaces = new UnityGameBoardSpace[quantizedBoard.NodesCount];

        yield break;
    }
}
