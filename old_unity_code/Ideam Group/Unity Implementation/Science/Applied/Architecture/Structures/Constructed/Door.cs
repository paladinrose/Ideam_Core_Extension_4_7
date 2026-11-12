using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif

using Ideam.Science.Applied.Architecture;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Door : Aperture, IDoor
    {
        public enum DoorHingeDirection
        {
            SwingToA,
            SwingToB,
            SwingBothWays
        }

        public OcclusionPortal occlusionPortal;


        public Transform door, doorHandle;
        public HingeJoint doorHinge;
        public bool handleOnTheRight;
        public DoorHingeDirection hingeDirection;

        public BoxCollider leftSideCollider, rightSideCollider, topCollider;

        public event Action<IThing> OnTraverse;

        public override void UpdateAperture()
        {
            Bounds2 apertureBounds = apertureShape.Extents;
            Float2 newApertureSize = apertureBounds.Size;
            //1. Move left and right colliders to apertureExtents.x and its negative, plus and minus half the collider size.
            Float2 apertureExtents = newApertureSize * 0.5f;

            Vector3 newPos = Vector3.zero;
            Vector3 newSize = Vector3.one;
            newSize.y = newApertureSize.y;

            if (leftSideCollider)
            {
                Transform l = leftSideCollider.transform, r = rightSideCollider.transform;
                newPos.x = -apertureExtents.x - leftSideCollider.size.x * 0.5f;

                //Now scale their Y to newAperatureSize.y and move the collider position to apertureExtents.y

                newPos.y = apertureExtents.y + leftSideCollider.size.y * 0.5f;
                l.localScale = newSize;
                l.localPosition = newPos;
            }

            if (rightSideCollider)
            {
                Transform r = rightSideCollider.transform;
                newPos.x = apertureExtents.x + rightSideCollider.size.x * 0.5f;
                newPos.y = apertureExtents.y + rightSideCollider.size.y * 0.5f;
                r.localScale = newSize;
                r.localPosition = newPos;
            }

            //2. Now we need to stretch the top and threshold.
            if (topCollider)
            {
                Transform t = topCollider.transform;
                newPos = Vector3.zero; newPos.y = newApertureSize.y - topCollider.size.y * 0.5f;

                newSize = Vector3.one; newSize.x = newApertureSize.x;
                t.localScale = newSize;
                t.localPosition = newPos;
            }

            //Next, we need to place the hinge midway up one side or the other.
            if (doorHinge)
            {
                Transform dh = doorHinge.transform;

                if (handleOnTheRight && leftSideCollider)
                {
                    dh.localPosition = new Vector3(leftSideCollider.transform.localPosition.x + leftSideCollider.size.x * 0.5f, apertureExtents.y, 0);
                }
                else if (!handleOnTheRight && rightSideCollider)
                {
                    dh.localPosition = new Vector3(rightSideCollider.transform.localPosition.x - rightSideCollider.size.x * 0.5f, apertureExtents.y, 0);

                }

            }

            //And we resize the physical door, itself.
            if (door)
            {
                switch (hingeDirection)
                {
                    case DoorHingeDirection.SwingToA:
                        //Move door.z to directionToA * doorBounds.z*0.5f;
                        break;

                    case DoorHingeDirection.SwingToB:

                        break;

                    case DoorHingeDirection.SwingBothWays:
                        //Put door.z to 0.
                        break;
                }
            }

            //Afterthought, resize the occlusion portal if we've got one.
            if (occlusionPortal)
            {
                Transform opt = occlusionPortal.transform;
                opt.localPosition = new Vector3(0, apertureExtents.y, 0);

#if UNITY_EDITOR
                SerializedObject op = new SerializedObject(occlusionPortal);
                op.Update();

                string k_SizePath = "m_Size";
                SerializedProperty sizeProperty = op.FindProperty(k_SizePath);

                Vector3 occluderSize = sizeProperty.vector3Value;
                occluderSize.x = newApertureSize.x;
                occluderSize.y = newApertureSize.y;

                sizeProperty.vector3Value = occluderSize;
                op.ApplyModifiedProperties();
#endif
            }
            //base.UpdateAperture(newApertureSize);
        }

        public bool Traverse<T>(T thing, int fromPlace, int toPlace) where T : IThing
        {
            throw new NotImplementedException();
        }
    }
}