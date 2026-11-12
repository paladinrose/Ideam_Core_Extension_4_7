using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;

public class PathFollower : MonoBehaviour
{
    public BoundPath[] boundPaths = new BoundPath[0];

    public void SetToPath(int id, float percent)
    {
        boundPaths[id].SetToPath(percent);
    }
}

[System.Serializable]
public class BoundPath
{
    public Spline3 path = new Spline3(3, CurveType.CatmullRom);
    public Transform pathSpace = null, boundToPath;

    public void SetToPath(float percent)
    {
        Vector3 pos = path.GetPoint(percent).ToVector3();
        if (pathSpace) pos = pathSpace.TransformPoint(pos);
        boundToPath.position = pos;
    }
}
