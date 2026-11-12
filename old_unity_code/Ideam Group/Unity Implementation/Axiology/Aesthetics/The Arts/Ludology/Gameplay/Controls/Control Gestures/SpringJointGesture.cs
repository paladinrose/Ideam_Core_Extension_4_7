using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;

[System.Serializable]
public class SpringJointGesture
{
    public SpringJoint springJoint;
    public Spline3 anchorSpline;
    public Spline1 springSpline;
    public Spline1 damperSpline;
    public Spline1 massScaleSpline;
    public Spline2 minMaxDistanceSpline;
    public void Gesture(float t)
    {
        springJoint.anchor = anchorSpline.GetPoint(t).ToVector3();

        springJoint.damper = damperSpline.GetPoint(t);

        springJoint.massScale = massScaleSpline.GetPoint(t);

        Vector2 minMax = minMaxDistanceSpline.GetPoint(t).ToVector2();
        springJoint.minDistance = minMax.x;
        springJoint.maxDistance = minMax.y;

        springJoint.spring = springSpline.GetPoint(t);

    }
}

