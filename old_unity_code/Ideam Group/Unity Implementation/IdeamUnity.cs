using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;

using UnityEngine;
using UnityEngine.SceneManagement;

using Unity.Mathematics;

using Ideam.Science.Formal.Mathematics;

#if UNITY_EDITOR
using UnityEditor;
#endif

using Bounds = UnityEngine.Bounds;
using Quaternion = UnityEngine.Quaternion;

public static class IdeamUnity 
{
    public enum BoundSides
    {
        Front, Back, Left, Right, Top, Bottom
    }

    #region Extension Methods

    #region GameObject Extensions

    public static GameObject Instantiate(GameObject source)
    {
        GameObject newObject = null;
        if (!Application.isEditor)
        {
            newObject = (GameObject)UnityEngine.Object.Instantiate(source,
                source.transform.position, source.transform.rotation);
        }
#if UNITY_EDITOR
        else
        {

            newObject = (GameObject)PrefabUtility.InstantiatePrefab(source,
                UnityEngine.SceneManagement.SceneManager.GetActiveScene());

        }
#endif
        return newObject;
    }

    public static T[] GetComponents<T>(this Scene scene, bool includeInactive = false) where T : UnityEngine.Component
    {
        List<T> components = new List<T>();
        GameObject[] sceneObjects = scene.GetRootGameObjects();

        for(int i =0; i < sceneObjects.Length; i++)
        {
            components.AddRange(sceneObjects[i].GetComponentsInChildren<T>(includeInactive));
        }

        return components.ToArray();
    } 

    public static T GetComponentInAncestry<T>(this GameObject source, bool includeInactive = false) where T : UnityEngine.Object
    {
        Transform ancestorWalk = source.transform;
        while (ancestorWalk.parent != null)
        {
            ancestorWalk = ancestorWalk.parent;
        }
        return ancestorWalk.GetComponentInChildren<T>(includeInactive);
    }

    public static T[] GetComponentsInAncestry<T>(this GameObject source, bool includeInactive = false) where T : UnityEngine.Object
    {
        Transform ancestorWalk = source.transform;
        while (ancestorWalk.parent != null)
        {
            ancestorWalk = ancestorWalk.parent;
        }
        return ancestorWalk.GetComponentsInChildren<T>(includeInactive);
    }

    public static void Destroy(UnityEngine.Object ob)
    {
        if (!Application.isEditor)
        {
            UnityEngine.Object.Destroy(ob);
        }
#if UNITY_EDITOR
        else
        {
            UnityEngine.Object.DestroyImmediate(ob);
        }
#endif
    }

    
    #endregion

    #region Transform Extensions

    public static Transform[] GetAllChildren(this Transform t)
    {
        List<Transform> kids = new List<Transform>();
        foreach (Transform kid in t)
        {
            kids.Add(kid);
        }
        return kids.ToArray();
    }

    public static int Depth(this Transform t)
    {
        int d = 0;
        while(t.parent != null)
        {
            d++;
            t = t.parent;
        }
        return d;
    }

    public static bool IsDescendentOf(this Transform t, Transform ancestor)
    {
        Transform parent = t.parent;
        while (parent)
        {
            if (parent == ancestor) { return true; }
            parent = parent.parent;
        }
        return false;
    }

    public static Bounds GlobalBounds(this Transform t, bool includeKids = false)
    {
        Bounds bounds;
        RectTransform rt;
        Renderer renderer;

        if (t is RectTransform)
        {
            rt = t as RectTransform;
            bounds = new Bounds(rt.rect.center, rt.TransformVector(rt.rect.size));
        }
        else
        {
            renderer = t.gameObject.GetComponent<Renderer>();
            if (renderer)
            {
                bounds = renderer.bounds;
            }
            else
            {
                bounds = new Bounds(t.position, Vector3.zero);
            }
        }

        if (includeKids)
        {
            Transform[] kids = t.gameObject.GetComponentsInChildren<Transform>();
            for (int i = 1; i < kids.Length; i++)
            {
                if (kids[i] is RectTransform)
                {
                    rt = kids[i] as RectTransform;
                    bounds.Encapsulate(new Bounds(rt.rect.center, rt.TransformVector(rt.rect.size)));
                }
                else
                {
                    renderer = kids[i].gameObject.GetComponent<Renderer>();
                    if (renderer)
                    {
                        bounds.Encapsulate(renderer.bounds);
                    }
                }
            }
        }
        return bounds;
    }

    public static Bounds LocalBounds(this Transform t, bool includeKids = false)
    {

        Bounds combinedBounds;
        Vector3 initRot = t.eulerAngles, initPos = t.position;
        Transform oldParent = t.parent;
        RectTransform rt;
        Renderer renderer;
        int oldID = -1;
        if (oldParent)
        {
            oldID = t.GetSiblingIndex();
        }

        t.SetParent(null, true);
        t.eulerAngles = t.position = Vector3.zero;

        if (t is RectTransform)
        {
            rt = t as RectTransform;
            combinedBounds = new Bounds(rt.rect.center, rt.TransformVector(rt.rect.size));
            //combinedBounds = new Bounds(rt.rect.center, rt.rect.size);
        }
        else
        {
            renderer = t.gameObject.GetComponent<Renderer>();
            if (renderer)
            {
                combinedBounds = renderer.bounds;
            }
            else
            {
                combinedBounds = new Bounds(Vector3.zero, Vector3.zero);
            }
        }

        if (includeKids)
        {
            Transform[] kids = t.gameObject.GetComponentsInChildren<Transform>();
            for (int i = 1; i < kids.Length; i++)
            {
                if (kids[i] is RectTransform)
                {
                    rt = kids[i] as RectTransform;
                    combinedBounds.Encapsulate(new Bounds(rt.rect.center, rt.TransformVector(rt.rect.size)));
                }
                else
                {
                    renderer = kids[i].gameObject.GetComponent<Renderer>();
                    if (renderer)
                    {
                        combinedBounds.Encapsulate(renderer.bounds);
                    }
                }
            }
        }

        if (oldParent)
        {
            t.SetParent(oldParent, true);
            t.SetSiblingIndex(oldID);
        }

        t.eulerAngles = initRot;
        t.position = initPos;
        Vector3 size = combinedBounds.size;
        if (t.localScale.x < 0)
        {
            size.x *= -1;
        }
        if (t.localScale.y < 0)
        {
            size.y *= -1;
        }
        if (t.localScale.z < 0)
        {
            size.z *= -1;
        }
        combinedBounds.size = size;

        return combinedBounds;
    }

    public static Bounds PhysicsLocalBounds(this Transform t, bool includeKids = false)
    {
        Bounds combinedBounds = new Bounds(Vector3.zero, Vector3.zero);
        Vector3 initRot = t.eulerAngles, initPos = t.position;
        Transform oldParent = t.parent;

        Collider[] colliders;

        t.SetParent(null, true);

        if (!includeKids)
        {
            colliders = new Collider[1];
            colliders[0] = t.gameObject.GetComponent<Collider>();
        }
        else
        {
            colliders = t.GetComponentsInChildren<Collider>();
        }
        for (int i = colliders.Length - 1; i >= 0; i--)
        {

            if (!colliders[i].isTrigger)
            {
                combinedBounds.Encapsulate(colliders[i].bounds);
            }
        }

        t.SetParent(oldParent, true);
        t.eulerAngles = initRot;
        t.position = initPos;
        Vector3 size = combinedBounds.size;
        if (t.localScale.x < 0)
        {
            size.x *= -1;
        }
        if (t.localScale.y < 0)
        {
            size.y *= -1;
        }
        if (t.localScale.z < 0)
        {
            size.z *= -1;
        }
        combinedBounds.size = size;

        return combinedBounds;
    }

    public static Bounds Physics2DLocalBounds(this Transform t, bool includeKids = false)
    {
        Bounds combinedBounds = new Bounds(Vector3.zero, Vector3.zero);
        Vector3 initRot = t.eulerAngles, initPos = t.position;
        Transform oldParent = t.parent;

        Collider2D[] colliders;

        t.SetParent(null, true);
        t.eulerAngles = t.position = Vector3.zero;

        if (!includeKids)
        {
            colliders = new Collider2D[1];
            colliders[0] = t.gameObject.GetComponent<Collider2D>();
        }
        else
        {
            colliders = t.GetComponentsInChildren<Collider2D>();
        }
        for (int i = colliders.Length - 1; i >= 0; i--)
        {

            if (!colliders[i].isTrigger)
            {
                combinedBounds.Encapsulate(colliders[i].bounds);
            }
        }

        t.SetParent(oldParent, true);
        t.eulerAngles = initRot;
        t.position = initPos;
        Vector3 size = combinedBounds.size;
        if (t.localScale.x < 0)
        {
            size.x *= -1;
        }
        if (t.localScale.y < 0)
        {
            size.y *= -1;
        }
        if (t.localScale.z < 0)
        {
            size.z *= -1;
        }
        combinedBounds.size = size;

        return combinedBounds;
    }

    public static Vector3 BoundsAlign(this Transform targ, Transform reference, Vector3 targAlign, Vector3 refAlign, Vector3 targOffset, Vector3 refOffset, bool includeKids = false)
    {

        Bounds referenceBounds = reference.LocalBounds(includeKids);

        Bounds targetBounds = targ.LocalBounds(includeKids);

        Vector3 referenceHalfSize = referenceBounds.size * 0.5f, targetHalfSize = targetBounds.size * 0.5f;
        referenceHalfSize -= refOffset;
        targetHalfSize -= targOffset;
        Vector3 alignmentPos = referenceBounds.center;
        alignmentPos += Vector3.Scale(refAlign, referenceHalfSize);
        alignmentPos -= Vector3.Scale(targAlign, targetHalfSize);

        return alignmentPos;
    }

    public static Vector3 LocalBoundsAlign(this Transform targ, Transform reference, Vector3 targAlign, Vector3 refAlign, Vector3 targOffset, Vector3 refOffset, bool includeKids = false)
    {

        Bounds targetBounds = targ.LocalBounds(includeKids);
        Bounds referenceBounds = reference.LocalBounds(includeKids);


        Vector3 referenceHalfSize = referenceBounds.size * 0.5f, targetHalfSize = targetBounds.size * 0.5f;
        referenceHalfSize -= refOffset;
        targetHalfSize -= targOffset;


        Vector3 currentPos = referenceBounds.center;
        Vector3 alignmentPos = Vector3.zero;

        referenceHalfSize -= refOffset;
        targetHalfSize -= targOffset;

        alignmentPos += Vector3.Scale(refAlign, referenceHalfSize);
        currentPos += reference.TransformDirection(alignmentPos);
        alignmentPos = Vector3.zero;
        alignmentPos -= Vector3.Scale(targAlign, targetHalfSize);
        currentPos += targ.TransformDirection(alignmentPos);
        return currentPos;
    }

    public static void SetLocalBounds(this Transform t, Bounds bounds, bool includeKids = false)
    {
        //Get the bounds right now.
        //Compare to the local scale.
        //Figure out how to scale to bring our bounds to where they should be.
        Bounds originalBounds = t.LocalBounds(includeKids);
        if (bounds == originalBounds)
        {
            return;
        }
        //Vector3  relativePosition = t.InverseTransformPoint (originalBounds.center);

        //desiredScale is to bounds.size as localscale is to originalBounds.size
        Vector3 localScale = t.localScale;
        localScale.x = bounds.size.x * localScale.x / originalBounds.size.x;
        localScale.y = bounds.size.y * localScale.y / originalBounds.size.y;
        localScale.z = bounds.size.z * localScale.z / originalBounds.size.z;
        t.localScale = localScale;
        originalBounds = t.LocalBounds(includeKids);
        Vector3 relativePosition = t.TransformDirection(originalBounds.center), relativeNew = t.TransformDirection(bounds.center);
        t.position += relativeNew - relativePosition;
        //t.position = relativeNew;
    }

    public static Vector3 LocalObjective(this Transform obj, Vector3 p)
    {
        return obj.position + obj.TransformDirection(p);
    }

    public static Vector3 InverseObjective(this Transform t, Vector3 p)
    {
        return t.InverseTransformDirection(p - t.position);
    }

    public static Vector3 LossyScale(this Transform obj, Vector3 size, bool includeKids = false)
    {
        Vector3 origScale = obj.localScale, newSize = Vector3.zero;
        Bounds bBounds = obj.LocalBounds(includeKids);
        //OriginalScale / bounds.size = newScale / newSize;
        //newScale = originalScale / bounds.size * newSize;
        float sideLength = origScale.x * size.x;
        if (sideLength == 0f || bBounds.size.x == 0f) { newSize.x = origScale.x; }
        else { newSize.x = sideLength / bBounds.size.x; }

        sideLength = origScale.y * size.y;
        if (sideLength == 0f || bBounds.size.y == 0f) { newSize.y = origScale.y; }
        else { newSize.y = sideLength / bBounds.size.y; }

        sideLength = origScale.z * size.z;
        if (sideLength == 0f || bBounds.size.z == 0f) { newSize.z = origScale.z; }
        else { newSize.z = sideLength / bBounds.size.z; }

        return newSize;
    }

    //I think this is more technically Pitch, Yaw, or Roll. I need to figure out which and rename it. 
    //I also need to change up the X, Y, and Z vals and create whichever other two I don't have.
    public static float PlanarAngle(this Transform t, Transform d)
    {
        Vector3 vec = Vector3.zero;
        vec = d.position - t.position;
        vec = d.InverseTransformDirection(vec);
        return Mathf.Atan2(vec.y, vec.x) * Mathf.Rad2Deg;
    }

    public static Vector3 ConvertEulerLocalToGlobal(this Transform targ, Vector3 localEulers)
    {
        Vector3 r = targ.eulerAngles;
        targ.localEulerAngles = targ.localEulerAngles + localEulers;
        Vector3 ret = targ.eulerAngles - r;
        targ.eulerAngles = r;
        return ret;
    }

    public static void MoveIndependent(this Transform targ, Vector3 moveTo)
    {
        Vector3 diff = (moveTo - targ.position);
        targ.position = moveTo;
        for(int i=0; i < targ.childCount; i++)
        {
            targ.GetChild(i).position -= diff;
        }
        
    }

    public static void RotateIndependent(this Transform targ, Quaternion rotation)
    {
        Transform[] kids = new Transform[targ.childCount];

        for(int i = kids.Length-1; i >=0; i--)
        {
            kids[i] = targ.GetChild(i);
            kids[i].parent = null;
        }

        targ.rotation = rotation;
        for(int i=0; i < kids.Length; i++)
        {
            kids[i].SetParent(targ, true);
        }
    }

    public static Vector3[] TransformPoints(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.TransformPoint(points[i]);
        }
        return points;
    }

    public static Vector3[] InverseTransformPoints(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.InverseTransformPoint(points[i]);
        }
        return points;
    }

    public static Vector3[] TransformDirections(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.TransformDirection(points[i]);
        }
        return points;
    }

    public static Vector3[] InverseTransformDirections(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.InverseTransformDirection(points[i]);
        }
        return points;
    }

    public static Vector3[] TransformVectors(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.TransformVector(points[i]);
        }
        return points;
    }

    public static Vector3[] InverseTransformVectors(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.InverseTransformVector(points[i]);
        }
        return points;
    }

    public static Vector3[] LocalObjectives(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.LocalObjective(points[i]);
        }
        return points;
    }

    public static Vector3[] InverseObjectives(this Transform t, Vector3[] points)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = t.InverseObjective(points[i]);
        }
        return points;
    }
    #endregion

    #region RectTransform Extensions
    
    public static void SetLeft(this RectTransform rt, float left)
    {
        rt.offsetMin = new Vector2(left, rt.offsetMin.y);
    }

    public static void SetRight(this RectTransform rt, float right)
    {
        rt.offsetMax = new Vector2(-right, rt.offsetMax.y);
    }
    public static void MoveX(this RectTransform rt, float x)
    {
        rt.SetLeft(x);
        rt.SetRight(-x);
    }

    public static void SetTop(this RectTransform rt, float top)
    {
        rt.offsetMax = new Vector2(rt.offsetMax.x, -top);
    }

    public static void SetBottom(this RectTransform rt, float bottom)
    {
        rt.offsetMin = new Vector2(rt.offsetMin.x, bottom);
    }

    public static void MoveY(this RectTransform rt, float y)
    {
        rt.SetTop(-y);
        rt.SetBottom(y);
    }
    #endregion

    #region Quaternion Extensions

    public static Quaternion Average(this Quaternion q, Quaternion other)
    {
        return Quaternion.Slerp(q, other, 0.5f);
    }
    public static Quaternion Average(this Quaternion q, Quaternion[] toAverage)
    {
        Quaternion average = new Quaternion(q.x, q.y, q.z, q.w);

        int amount = 0;

        foreach (var quaternion in toAverage)
        {
            amount++;

            average = Quaternion.Slerp(average, quaternion, 1f / amount);
        }
        return average;
    }
    #endregion

    #region Rect Extensions

    public static Rect Encompass(this Rect a, Rect b)
    {
        Rect newRect = new Rect(a);
        if (b.xMin < a.xMin)
        {
            newRect.xMin = b.xMin;
        }

        if (b.yMin < a.yMin)
        {
            newRect.yMin = b.yMin;
        }

        if (b.xMax > a.xMax)
        {
            newRect.xMax = b.xMax;
        }

        if (b.yMax > a.yMax)
        {
            newRect.yMax = b.yMax;
        }
        return newRect;
    }

    public static Rect Encompass(this Rect r, Vector2 point)
    {
        Rect rr = new Rect(r);
        if (point.x > rr.xMax) { rr.xMax = point.x; }
        else if (point.x < rr.xMin) { r.xMin = point.x; }

        if (point.y > rr.yMax) { rr.yMax = point.y; }
        else if (point.y < rr.yMin) { rr.yMin = point.y; }
        return rr;
    }
    #endregion

    #region Collider Extensions
    public static Bounds Bounds(this Collider2D[] colliders)
    {
        Bounds combinedBounds = colliders[colliders.Length - 1].bounds;

        for (int i = colliders.Length - 2; i >= 0; i--)
        {

            if (!colliders[i].isTrigger)
            {
                combinedBounds.Encapsulate(colliders[i].bounds);
            }
        }


        return combinedBounds;
    }

    public static bool Contains(this Collider c, Vector3 point)
    {

        if (c.bounds.Contains(point))
        {
            Vector3 start = c.bounds.max * 2, direction = point - start;
            direction.Normalize();
            int i = 0;
            Vector3 currentPoint = start;
            while (currentPoint != point)
            {
                RaycastHit hit;
                if (c.Raycast(new Ray(currentPoint, direction), out hit, 1000))
                {
                    i++;
                    currentPoint = hit.point + (direction * 0.0001f);
                }
                else
                {
                    currentPoint = point;
                }
            }

            while (currentPoint != start)
            {
                RaycastHit hit;
                if (c.Raycast(new Ray(currentPoint, -direction), out hit, 1000))
                {
                    i++;
                    currentPoint = hit.point + (-direction * 0.0001f);
                }
                else
                {
                    currentPoint = start;
                }
            }

            if (i % 2 == 1)
            {
                return true;
            }
        }
        return false;
    }

    public static bool Contains(this Collider[] c, Vector3 point, ref int colliderID)
    {
        for (int i = c.Length - 1; i >= 0; i--)
        {
            if (c[i].Contains(point))
            {
                colliderID = i;
                return true;
            }
        }
        colliderID = -1;
        return false;
    }

    
    #endregion

    #region Configurable Joint Extensions
    public static void SetTargetRotationLocal(this ConfigurableJoint joint, Quaternion targetLocalRotation, Quaternion startLocalRotation)
    {
        if (joint.configuredInWorldSpace)
        {
            Debug.LogError("SetTargetRotationLocal should not be used with joints that are configured in world space. For world space joints, use SetTargetRotation.", joint);
        }
        SetTargetRotationInternal(joint, targetLocalRotation, startLocalRotation, Space.Self);
    }

    /// <summary>
    /// Sets a joint's targetRotation to match a given world rotation.
    /// The joint transform's world rotation must be cached on Start and passed into this method.
    /// </summary>
    public static void SetTargetRotation(this ConfigurableJoint joint, Quaternion targetWorldRotation, Quaternion startWorldRotation)
    {
        if (!joint.configuredInWorldSpace)
        {
            Debug.LogError("SetTargetRotation must be used with joints that are configured in world space. For local space joints, use SetTargetRotationLocal.", joint);
        }
        SetTargetRotationInternal(joint, targetWorldRotation, startWorldRotation, Space.World);
    }

    static void SetTargetRotationInternal(ConfigurableJoint joint, Quaternion targetRotation, Quaternion startRotation, Space space)
    {
        // Calculate the rotation expressed by the joint's axis and secondary axis
        var right = joint.axis;
        var forward = Vector3.Cross(joint.axis, joint.secondaryAxis).normalized;
        var up = Vector3.Cross(forward, right).normalized;
        Quaternion worldToJointSpace = Quaternion.LookRotation(forward, up);

        // Transform into world space
        Quaternion resultRotation = Quaternion.Inverse(worldToJointSpace);

        // Counter-rotate and apply the new local rotation.
        // Joint space is the inverse of world space, so we need to invert our value
        if (space == Space.World)
        {
            resultRotation *= startRotation * Quaternion.Inverse(targetRotation);
        }
        else
        {
            resultRotation *= Quaternion.Inverse(targetRotation) * startRotation;
        }

        // Transform back into joint space
        resultRotation *= worldToJointSpace;

        // Set target rotation to our newly calculated rotation
        joint.targetRotation = resultRotation;
    }
    #endregion

    #region Bounds Extensions 
    public static Bounds MoveSide(this Bounds b, BoundSides side, float dist)
    {
        Vector3 center = b.center, size = b.size;
        switch (side)
        {
            case BoundSides.Front:
                size.z = b.max.z + dist - b.min.z;
                center.z = b.min.z + size.z * 0.5f;
                break;
            case BoundSides.Back:
                size.z = b.max.z - b.min.z + dist;
                center.z = b.max.z - size.z * 0.5f;
                break;
            case BoundSides.Left:
                size.x = b.max.x - b.min.x + dist;
                center.x = b.max.x - size.x * 0.5f;
                break;
            case BoundSides.Right:
                size.x = b.max.x + dist - b.min.x;
                center.x = b.min.x + size.x * 0.5f;
                break;
            case BoundSides.Top:
                size.y = b.max.y + dist - b.min.y;
                center.y = b.min.y + size.y * 0.5f;
                break;
            case BoundSides.Bottom:
                size.y = b.max.y + dist - b.min.y;
                center.y = b.max.y - size.y * 0.5f;
                break;
        }
        return new Bounds(center, size);
    }

    public static Bounds SetSide(this Bounds b, BoundSides side, float value)
    {
        Vector3 center = b.center, size = b.size;
        switch (side)
        {
            case BoundSides.Front:
                size.z = value - b.min.z;
                center.z = b.min.z + size.z * 0.5f;
                break;
            case BoundSides.Back:
                size.z = b.max.z - value;
                center.z = b.max.z - size.z * 0.5f;
                break;
            case BoundSides.Left:
                size.x = b.max.x - value;
                center.x = b.max.x - size.x * 0.5f;
                break;
            case BoundSides.Right:
                size.x = value - b.min.x;
                center.x = b.min.x + size.x * 0.5f;
                break;
            case BoundSides.Top:
                size.y = value - b.min.y;
                center.y = b.min.y + size.y * 0.5f;
                break;
            case BoundSides.Bottom:
                size.y = b.max.y - value;
                center.y = b.max.y - size.y * 0.5f;
                break;
        }
        
        return new Bounds(center, size);
    }
    public static float Volume(this Bounds b)
    {
        return b.size.x * b.size.y * b.size.z;
    }
    #endregion

    #region Vectors and Floats

    public static bool TryParse(this Vector2 v, string toParse, out Vector2 parsed)
    {
        string[] parseParts = toParse.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
        if (parseParts.Length >= 2)
        {
            if (float.TryParse(parseParts[0], out float x))
            {
                if (float.TryParse(parseParts[1], out float y))
                {
                    parsed = new Vector2(x, y);
                    return true;
                }
            }
        }
        parsed = Vector2.zero;
        return false;
    }
    public static bool TryParse(this Vector3 v, string toParse, out Vector3 parsed)
    {
        string[] parseParts = toParse.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
        if (parseParts.Length >= 3)
        {
            if (float.TryParse(parseParts[0], out float x))
            {
                if (float.TryParse(parseParts[1], out float y))
                {
                    if (float.TryParse(parseParts[2], out float z))
                    {
                        parsed = new Vector3(x, y, z);
                        return true;
                    }
                }
            }
        }
        parsed = Vector3.zero;
        return false;
    }

    public static Vector2Int ToVector2Int(this Vector2 p)
    {
        return new Vector2Int(Mathf.FloorToInt(p.x), Mathf.FloorToInt(p.y));
    }

    public static Vector2[] Normalize(this Vector2[] vec)
    {
        Vector2[] newVecs = new Vector2[vec.Length];
        Rect bounds = new Rect();
        for (int i = vec.Length - 1; i >= 0; i--)
        {
            if (vec[i].x < bounds.xMin) { bounds.xMin = vec[i].x; }
            else if (vec[i].x > bounds.xMax) { bounds.xMax = vec[i].x; }
            if (vec[i].y < bounds.yMin) { bounds.yMin = vec[i].y; }
            else if (vec[i].y > bounds.yMax) { bounds.yMax = vec[i].y; }
        }
        Vector2 scl = new Vector2(1f / bounds.size.x, 1f / bounds.size.y),
            offset = Vector2.zero;
        if (bounds.xMin < 0) { offset.x = bounds.xMin * scl.x * -1; }
        if (bounds.yMin < 0) { offset.y = bounds.yMin * -1 * scl.y; }

        for (int i = vec.Length - 1; i >= 0; i--)
        {
            newVecs[i] = new Vector2((vec[i].x * scl.x) + offset.x, (vec[i].y * scl.y) + offset.y);
        }
        return newVecs;
    }

    public static bool InTolerance(this Vector3 a, Vector3 b, float tol)
    {
        Vector3 d = b - a;
        if (Mathf.Abs(d.x) > tol)
        {
            return false;
        }
        if (Mathf.Abs(d.y) > tol)
        {
            return false;
        }
        if (Mathf.Abs(d.z) > tol)
        {
            return false;
        }
        return true;
    }

    public static bool InTolerance(this float3 a, float3 b, float tol)
    {
        float3 d = b - a;
        if (math.abs(d.x) > tol)
        {
            return false;
        }
        if (math.abs(d.y) > tol)
        {
            return false;
        }
        if (math.abs(d.z) > tol)
        {
            return false;
        }
        return true;
    }

    public static bool InTolerance(this Vector2 a, Vector2 b, float tol)
    {
        Vector2 d = b - a;

        if (Mathf.Abs(d.x) > tol)
        {
            return false;
        }
        if (Mathf.Abs(d.y) > tol)
        {
            return false;
        }
        return true;
    }

    public static bool InTolerance(this float2 a, float2 b, float tol)
    {
        float2 d = b - a;
        if (math.abs(d.x) > tol)
        {
            return false;
        }
        if (math.abs(d.y) > tol)
        {
            return false;
        }
        return true;
    }

    public static bool InTolerance(this float a, float b, float tol)
    {
        float d = b - a;
        if (math.abs(d) > tol)
        {
            return false;
        }

        return true;
    }


    public static Vector3 ToVector3(this Vector2 vec2, XYToXYZ conversion)
    {
        switch (conversion)
        {
            case XYToXYZ.XZ: return new Vector3(vec2.x, 1, vec2.y);
            case XYToXYZ.YZ: return new Vector3(1, vec2.x, vec2.y);
        }
        return new Vector3(vec2.x, vec2.y, 1);
    }
    public static Vector3 Vector3XZ(this Vector2 vec2)
    {
        return new Vector3(vec2.x, 0, vec2.y);
    }

    public static Vector3 Vector3YZ(this Vector2 vec2)
    {
        return new Vector3(0, vec2.y, vec2.x);
    }

    public static Vector2 Vector2XZ(this Vector3 vec3)
    {
        return new Vector2(vec3.x, vec3.z);
    }

    public static Vector2 Vector2ZY(this Vector3 vec3)
    {
        return new Vector2(vec3.z, vec3.y);
    }



    public static Vector2[] ToVector2(this Vector3[] value)
    {
        Vector2[] v = new Vector2[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }

    public static Vector2[] ToVector2(this float3[] value)
    {
        Vector2[] v = new Vector2[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = new Vector2(value[i].x, value[i].y);
        }
        return v;
    }

    public static Vector2[] ToVector2(this float2[] value)
    {
        Vector2[] v = new Vector2[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }


    public static Vector3[] ToVector3(this float2[] value)
    {
        Vector3[] v = new Vector3[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = new Vector3(value[i].x, value[i].y, 0);
        }
        return v;
    }

    public static Vector3[] ToVector3(this Vector2[] value)
    {
        Vector3[] v = new Vector3[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }

    public static Vector3[] ToVector3(this float3[] value)
    {
        Vector3[] v = new Vector3[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }



    public static float2[] Tofloat2(this float3[] value)
    {
        float2[] v = new float2[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = new float2(value[i].x, value[i].y);
        }
        return v;
    }

    public static float2[] Tofloat2(this Vector2[] value)
    {
        float2[] v = new float2[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }


    public static float3[] Tofloat3(this float2[] value)
    {
        float3[] v = new float3[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = new float3(value[i].x, value[i].y, 0);
        }
        return v;
    }

    public static float3[] Tofloat3(this Vector3[] value)
    {
        float3[] v = new float3[value.Length];
        for (int i = v.Length - 1; i >= 0; i--)
        {
            v[i] = value[i];
        }
        return v;
    }


    public static float TriangleArea(this Vector2 p1, Vector2 p2, Vector2 p3)
    {
        // calculate signed triangle area using a kind of "2D cross product":
        Vector2 v1 = p1 - p3;
        Vector2 v2 = p2 - p3;
        return ((v1.x * v2.y - v1.y * v2.x) * 0.5f);
    }


    public static int GetClosestPoint(this Vector2[] _points, Vector3 p)
    {
        float lowestDist = float.PositiveInfinity;
        int id = -1;
        for (int i = _points.Length - 1; i >= 0; i--)
        {
            float d = Vector3.Distance(p, _points[i]);
            if (d < lowestDist)
            {
                id = i;
                lowestDist = d;
            }
        }
        return id;
    }


    public static Vector2 RotateAround(this Vector2 point, Vector2 pivot, float angle)
    {
        if (angle == 0) { return point; }
        Vector2 p = point - pivot;
        float rad = angle * Mathf.Deg2Rad;
        float s = Mathf.Sin(rad);
        float c = Mathf.Cos(rad);
        return pivot + new Vector2(p.x * c - p.y * s, p.y * c + p.x * s);

    }

    public static Vector2[] RotateAround(this Vector2[] points, Vector2 pivot, float angle)
    {
        if (angle == 0) { return points; }
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = points[i].RotateAround(pivot, angle);
        }
        return points;
    }

    public static Vector3 RotateAround(this Vector3 point, Vector3 pivot, Vector3 angles)
    {
        Vector3 dir = point - pivot; // get point direction relative to pivot
        dir = Quaternion.Euler(angles) * dir; // rotate it
        point = dir + pivot; // calculate rotated point
        return point; // return it
    }

    public static Vector3[] RotateAround(this Vector3[] points, Vector3 pivot, Vector3 angles)
    {
        if (angles == Vector3.zero) { return points; }
        for (int i = points.Length - 1; i >= 0; i--)
        {
            Vector3 dir = points[i] - pivot; // get point direction relative to pivot
            dir = Quaternion.Euler(angles) * dir; // rotate it
            points[i] = dir + pivot; // calculate rotated point
        }
        return points;
    }

    public static float2 RotateAround(this float2 point, float2 pivot, float angle)
    {
        if (angle == 0) { return point; }
        float2 p = point - pivot;
        float rad = angle * Mathf.Deg2Rad;
        float s = Mathf.Sin(rad);
        float c = Mathf.Cos(rad);
        return pivot + new float2(p.x * c - p.y * s, p.y * c + p.x * s);

    }

    public static float2[] RotateAround(this float2[] points, float2 pivot, float angle)
    {
        if (angle == 0) { return points; }
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = points[i].RotateAround(pivot, angle);
        }
        return points;
    }

    public static float3 RotateAround(this float3 point, float3 pivot, float3 angles)
    {
        float3 dir = point - pivot; // get point direction relative to pivot
        dir = Quaternion.Euler(angles) * dir; // rotate it
        point = dir + pivot; // calculate rotated point
        return point; // return it
    }

    public static float3[] RotateAround(this float3[] points, float3 pivot, float3 angles)
    {
        if (angles.Equals(Vector3.zero)) { return points; }
        for (int i = points.Length - 1; i >= 0; i--)
        {
            float3 dir = points[i] - pivot; // get point direction relative to pivot
            dir = Quaternion.Euler(angles) * dir; // rotate it
            points[i] = dir + pivot; // calculate rotated point
        }
        return points;
    }


    public static Vector2[] Scale(this Vector2[] points, Vector2 scale)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i].Scale(scale);
        }
        return points;
    }

    public static Vector2[] Translate(this Vector2[] points, Vector2 offset)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] += offset;
        }
        return points;
    }


    public static float2[] Scale(this float2[] points, float2 scale)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] = points[i].Scale(scale);
        }
        return points;
    }

    public static float2[] Translate(this float2[] points, float2 offset)
    {
        for (int i = points.Length - 1; i >= 0; i--)
        {
            points[i] += offset;
        }
        return points;
    }



    public static Vector2 TranslateLocal(this Vector2 p, float forwardAngle, Vector2 translation)
    {
        Vector2 newP = new Vector2(Mathf.Cos(forwardAngle) * translation.x, Mathf.Sin(forwardAngle) * translation.y);
        return newP + p;
    }

    public static float2 TranslateLocal(this float2 p, float forwardAngle, float2 translation)
    {
        float2 newP = new float2(math.cos(forwardAngle) * translation.x, math.sin(forwardAngle) * translation.y);
        return newP + p;
    }


    public static float2 Scale(this float2 p, float2 scalar)
    {
        return new float2(p.x * scalar.x, p.y * scalar.y);
    }

    public static float3 Scale(this float3 p, float3 scalar)
    {
        return new float3(p.x * scalar.x, p.y * scalar.y, p.z * scalar.z);
    }
    public static float4 Scale(this float4 p, float4 scalar)
    {
        return new float4(p.x * scalar.x, p.y * scalar.y, p.z * scalar.z, p.w * scalar.w);
    }

    public static bool IsOnLine(this Vector2 p, Vector2 lineStart, Vector2 lineEnd)
    {
        float slope = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x);
        float yIntercept = lineStart.y - lineStart.x * slope;
        return ((slope * p.x + yIntercept) == (p.y));
    }

    public static bool LineIntersection(float2 pA1, float2 pA2, float2 pB1, float2 pB2, ref float2 intersection)
    {
        if (pA1.Equals(pB1) || pA1.Equals(pB2) || pA2.Equals(pB1) || pA2.Equals(pB2)) { return false; }

        float Ax, Bx, Cx, Ay, By, Cy, alphaNumerator, betaNumerator, denominator, intersectNumerator;
        float x1lo, x1hi, y1lo, y1hi;

        Ax = pA2.x - pA1.x;
        Bx = pB1.x - pB2.x;

        // X bound box test/

        if (Ax < 0) { x1lo = pA2.x; x1hi = pA1.x; }
        else { x1hi = pA2.x; x1lo = pA1.x; }

        if (Bx > 0) { if (x1hi < pB2.x || pB1.x < x1lo) { return false; } }
        else { if (x1hi < pB1.x || pB2.x < x1lo) { return false; } }

        Ay = pA2.y - pA1.y;
        By = pB1.y - pB2.y;

        // Y bound box test//

        if (Ay < 0) { y1lo = pA2.y; y1hi = pA1.y; }
        else { y1hi = pA2.y; y1lo = pA1.y; }

        if (By > 0) { if (y1hi < pB2.y || pB1.y < y1lo) { return false; } }
        else { if (y1hi < pB1.y || pB2.y < y1lo) { return false; } }

        Cx = pA1.x - pB1.x;
        Cy = pA1.y - pB1.y;

        // alpha tests

        alphaNumerator = By * Cx - Bx * Cy;
        denominator = Ay * Bx - Ax * By;

        if (denominator > 0) { if (alphaNumerator < 0 || alphaNumerator > denominator) { return false; } }
        else { if (alphaNumerator > 0 || alphaNumerator < denominator) { return false; } }

        // Beta tests 

        betaNumerator = Ax * Cy - Ay * Cx;

        if (denominator > 0) { if (betaNumerator < 0 || betaNumerator > denominator) { return false; } }
        else { if (betaNumerator > 0 || betaNumerator < denominator) { return false; } }

        // Check if they are parallel

        if (denominator == 0) { return false; }

        // Compute intersection coordinates 

        intersectNumerator = alphaNumerator * Ax;

        intersection.x = pA1.x + intersectNumerator / denominator;

        intersectNumerator = alphaNumerator * Ay;

        intersection.y = pA1.y + intersectNumerator / denominator;

        return true;
    }

    public static float2[] ReducePoints(this float2[] points, float angleTolerance, float minimumDeviation)
    {

        //Get the angle between our first two points.
        //Start going through from that point. Check the angle of the current against the next in line.
        //As long as it's the same, we don't record it. Once it's different, we record the last one, then start checking against this NEW angle.
        //We don't include our first one, since we check it with the last for a run: If the angle between the last and the first is the same as between the first and second, we're done, because the
        //last point we recorded started this run, and the first point we recorded ends it.
        float angle, currentAngle, angleDiff;
        float2 d;
        List<float2> newPoints = new List<float2>();
        d = points[1] - points[0];
        angle = Mathf.Atan2(d.y, d.x) * Mathf.Rad2Deg;

        for (int i = 1; i < points.Length; i++)
        {
            if (i < points.Length - 1)
            {
                d = points[i + 1] - points[i];
                currentAngle = math.atan2(d.y, d.x) * Mathf.Rad2Deg;
                angleDiff = angle - currentAngle;
                if (math.abs(angleDiff) > angleTolerance)
                {
                    //Check the next point. If it's back on angle, and the distance from our last
                    //to our next is less than minimumOffsetSize, skip straight on by this one.
                    if (i < points.Length - 2)
                    {
                        d = points[i + 2] - points[i - 1];
                        angleDiff = angle - (Mathf.Atan2(d.y, d.x) * Mathf.Rad2Deg);
                        if (math.abs(angleDiff) < angleTolerance)
                        {
                            if (math.distance(points[i + 2], points[i - 1]) >= minimumDeviation)
                            {
                                newPoints.Add(points[i]);
                                angle = currentAngle;
                            }
                            else
                            {
                                i++;
                            }
                        }
                        else
                        {
                            newPoints.Add(points[i]);
                            angle = currentAngle;
                        }

                    }
                    else
                    {
                        //Check against 0
                        d = points[0] - points[i];
                        angleDiff = angle - (math.atan2(d.y, d.x) * Mathf.Rad2Deg);
                        if (math.abs(angleDiff) < angleTolerance)
                        {
                            if (math.distance(points[0], points[i - 1]) >= minimumDeviation)
                            {
                                newPoints.Add(points[i]);
                                angle = currentAngle;
                            }
                            else
                            {
                                i++;
                                d = points[1] - points[0];
                                angleDiff = currentAngle - (math.atan2(d.y, d.x) * Mathf.Rad2Deg);
                                if (math.abs(angleDiff) > angleTolerance)
                                {
                                    newPoints.Add(points[0]);
                                }
                            }
                        }
                        else
                        {
                            newPoints.Add(points[i]);
                            angle = currentAngle;
                        }
                    }
                }
            }
            else
            {
                d = points[0] - points[i];
                currentAngle = math.atan2(d.y, d.x) * Mathf.Rad2Deg;
                angleDiff = angle - currentAngle;
                if (math.abs(angleDiff) >= angleTolerance)
                {
                    newPoints.Add(points[i]);
                }
                d = points[1] - points[0];
                angleDiff = currentAngle - (math.atan2(d.y, d.x) * Mathf.Rad2Deg);
                if (math.abs(angleDiff) > angleTolerance)
                {
                    newPoints.Add(points[0]);
                }
            }
        }
        return newPoints.ToArray();
    }


    public static float[] Tofloat(this Float[] flt)
    {
        float[] floats = new float[flt.Length];
        for (int i = 0; i < flt.Length; i++)
        {
            floats[i] = flt[i].x;
        }
        return floats;
    }
    public static Float[] ToFloat(this float[] flt)
    {
        Float[] floats = new Float[flt.Length];
        for (int i = 0; i < flt.Length; i++)
        {
            floats[i] = new Float(flt[i]);
        }
        return floats;
    }

    public static Vector2 ToVector2(this Float2 f2)
    {
        return new Vector2(f2.x, f2.y);
    }
    public static Vector2[] ToVector2(this Float2[] f2)
    {
        Vector2[] vec2s = new Vector2[f2.Length];
        for(int i =0; i < f2.Length; i++)
        {
            vec2s[i] =  new Vector2(f2[i].x, f2[i].y);
        }
        return vec2s;
    }

    public static Float2 ToFloat2(this Vector2 v2)
    {
        return new Float2(v2.x, v2.y);
    }
    public static Float2[] ToFloat2(this Vector2[] v2)
    {
        Float2[] float2s = new Float2[v2.Length];
        for (int i = 0; i < v2.Length; i++)
        {
            float2s[i] = new Float2(v2[i].x, v2[i].y);
        }
        return float2s;
    }

    public static Float2 ToFloat2(this Vector3 v2, XYToXYZ axisConversion = XYToXYZ.XY)
    {
        switch (axisConversion)
        {
            case XYToXYZ.XZ:
                return new Float2(v2.x, v2.z);

            case XYToXYZ.YZ:
                return new Float2(v2.y, v2.z);

            default:
                return new Float2(v2.x, v2.y);

        }
    }
    public static Float2[] ToFloat2(this Vector3[] v2, XYToXYZ axisConversion = XYToXYZ.XY)
    {
        Float2[] float2s = new Float2[v2.Length];
        for (int i = 0; i < v2.Length; i++)
        {
            switch (axisConversion)
            {
                case XYToXYZ.XZ:
                    float2s[i] = new Float2(v2[i].x, v2[i].z);
                    break;

                case XYToXYZ.YZ:
                    float2s[i] = new Float2(v2[i].y, v2[i].z);
                    break;

                default:
                    float2s[i] = new Float2(v2[i].x, v2[i].y);
                    break;

            }
        }
        return float2s;
    }


    public static Vector3 ToVector3(this Float2 f3, XYToXYZ axisConversion = XYToXYZ.XY)
    {
        switch (axisConversion)
        {
            case XYToXYZ.XZ:
                return new Vector3(f3.x, 0, f3.y);
                
            case XYToXYZ.YZ:
                return new Vector3(0, f3.x, f3.y);
            
            default:
                return new Vector3(f3.x, f3.y);
        }
    }

    public static Vector3[] ToVector3(this Float2[] f3, XYToXYZ axisConversion = XYToXYZ.XY)
    {
        Vector3[] vec3s = new Vector3[f3.Length];
        for (int i = 0; i < f3.Length; i++)
        {
            switch (axisConversion)
            {
                case XYToXYZ.XZ:
                   vec3s[i] = new Vector3(f3[i].x, 0, f3[i].y);
                    break;
                case XYToXYZ.YZ:
                    vec3s[i] = new Vector3(0, f3[i].x, f3[i].y);
                    break;
                default:
                    vec3s[i] = new Vector3(f3[i].x, f3[i].y);
                    break;
            }
        }
        return vec3s;
    }

    public static Vector3 ToVector3(this Float3 f3)
    {
        return new Vector3(f3.x, f3.y, f3.z);
    }
    public static Vector3[] ToVector3(this Float3[] f3)
    {
        Vector3[] vec3s = new Vector3[f3.Length];
        for (int i = 0; i < f3.Length; i++)
        {
            vec3s[i] = new Vector3(f3[i].x, f3[i].y, f3[i].z);
        }
        return vec3s;
    }
   
    public static Float3 ToFloat3(this Vector3 v3)
    {
        return new Float3(v3.x, v3.y, v3.z);
    }
    public static Float3[] ToFloat3(this Vector3[] v3)
    {
        Float3[] float3s = new Float3[v3.Length];
        for (int i = 0; i < v3.Length; i++)
        {
            float3s[i] = new Float3(v3[i].x, v3[i].y, v3[i].z);
        }
        return float3s;
    }


    public static Vector4 ToVector4(this Float4 f4)
    {
        return new Vector4(f4.x, f4.y, f4.z, f4.w);
    }
    public static Vector4[] ToVector4(this Float4[] f4)
    {
        Vector4[] vec4s = new Vector4[f4.Length];
        for (int i = 0; i < f4.Length; i++)
        {
            vec4s[i] = new Vector4(f4[i].x, f4[i].y, f4[i].z, f4[i].w);
        }
        return vec4s;
    }

    public static Float4 ToFloat4(this Vector4 v4)
    {
        return new Float4(v4.x, v4.y, v4.z, v4.w);
    }
    public static Float4[] ToFloat4(this Vector4[] v4)
    {
        Float4[] float4s = new Float4[v4.Length];
        for (int i = 0; i < v4.Length; i++)
        {
            float4s[i] = new Float4(v4[i].x, v4[i].y, v4[i].z, v4[i].w);
        }
        return float4s;
    }
    #endregion

    #region Mesh Component Extensions

    public static Vector3[] UvToWorldPoints(this MeshFilter m, Vector2 uv)
    {
        List<Vector3> hits = new List<Vector3>();
        Mesh mesh = m.mesh;
        int[] tris = mesh.triangles;
        Vector2[] uvs = mesh.uv;
        Vector3[] verts = mesh.vertices;
        for (int i = 0; i < tris.Length; i += 3)
        {

            Vector2 u1 = uvs[tris[i]];
            Vector2 u2 = uvs[tris[i + 1]];
            Vector2 u3 = uvs[tris[i + 2]];
            // calculate triangle area - if zero, skip it
            float a = u1.TriangleArea(u2, u3); if (a == 0) { continue; }
            // calculate barycentric coordinates of u1, u2 and u3
            // if anyone is negative, point is outside the triangle: skip it
            float a1 = u2.TriangleArea(u3, uv) / a; if (a1 < 0) { continue; }
            float a2 = u3.TriangleArea(u1, uv) / a; if (a2 < 0) { continue; }
            float a3 = u1.TriangleArea(u2, uv) / a; if (a3 < 0) { continue; }
            // point inside the triangle - find mesh position by interpolation...
            Vector3 p3D = a1 * verts[tris[i]] + a2 * verts[tris[i + 1]] + a3 * verts[tris[i + 2]];
            // and return it in world coordinates:
            hits.Add(m.transform.TransformPoint(p3D));
            //Now, add the normal of the triangle in question.
            hits.Add(m.mesh.normals[i / 3]);
        }

        return hits.ToArray();
    }

    public static Vector2 WorldPointToUV(this MeshCollider m, Vector3 worldPoint, Vector3 normal)
    {
        //world point has six dimensions, XYZ, and the NORMALS XYZ.
        RaycastHit hit;
        float rayLength = 0.1f;

        Ray ray = new Ray(worldPoint - normal * rayLength * 0.5f, normal);

        if (m.Raycast(ray, out hit, rayLength))
        {
            return hit.textureCoord;
        }
        return Vector2.zero;
    }
    #endregion

    #region Animator Extensions
    public static bool HasParameter(this Animator animator, string paramName)
    {
        foreach (AnimatorControllerParameter param in animator.parameters)
        {
            if (param.name == paramName)
                return true;
        }
        return false;
    }
    #endregion

    #region Color Extensions

    public static void SetHue(this Color c, float hue, bool hdr = false)
    {
        float h, s, v;
        Color.RGBToHSV(c, out h, out s, out v);

        c = Color.HSVToRGB(hue, s, v, hdr);
    }
    public static void SetSaturation(this Color c, float saturation, bool hdr = false)
    {
        float h, s, v;
        Color.RGBToHSV(c, out h, out s, out v);

        c = Color.HSVToRGB(h, saturation, v, hdr);
    }
    public static void SetValue(this Color c, float value, bool hdr = false)
    {
        float h, s, v;
        Color.RGBToHSV(c, out h, out s, out v);

        c = Color.HSVToRGB(h, s, value, hdr);
    }

    public static Float4[] ToFloat4(this Color[] colors)
    {
        Float4[] colVals = new Float4[colors.Length];
        for(int i =0; i < colVals.Length; i++)
        {
            colVals[i] = new Float4(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        }
        return colVals;
    }
    #endregion

    #region LayerMask Extensions

    public static bool Masked(this LayerMask l, int layer)
    {
        return (l == (l | (1 << layer)));
    }

    #endregion

    #endregion
}
