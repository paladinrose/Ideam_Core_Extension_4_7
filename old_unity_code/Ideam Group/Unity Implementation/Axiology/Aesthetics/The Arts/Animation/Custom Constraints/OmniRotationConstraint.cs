using System;
using System.Collections;
using System.Collections.Generic;

using Unity.Burst;
using Unity.Mathematics;

using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Animations.Rigging;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    public class OmniRotationConstraint : RigConstraint<OmniRotationJob, OmniRotationData, OmniRotationBinder>
    {
        public bool isLocal,
                    bindX, invertX,
                    bindY, invertY,
                    bindZ, invertZ;
        public Vector3 angleOffset;
    }

    [BurstCompile]
    public struct OmniRotationJob : IWeightedAnimationJob
    {
        public ReadWriteTransformHandle constrained;
        public ReadOnlyTransformHandle source;
        public Vector3 originalAngleOffset { get; set; }
        public BoolProperty isLocal { get; set; }
        public BoolProperty bindX { get; set; }
        public BoolProperty bindY { get; set; }
        public BoolProperty bindZ { get; set; }
        public BoolProperty invertX { get; set; }
        public BoolProperty invertY { get; set; }
        public BoolProperty invertZ { get; set; }
        public Vector3Property angleOffset { get; set; }
        public FloatProperty jobWeight { get; set; }

        public void ProcessRootMotion(AnimationStream stream)
        {

        }

        public void ProcessAnimation(AnimationStream stream)
        {
            float w = jobWeight.Get(stream);

            if (w > 0f)
            {
                bool iL = isLocal.Get(stream),
                     bX = bindX.Get(stream), iX = invertX.Get(stream),
                     bY = bindY.Get(stream), iY = invertY.Get(stream),
                     bZ = bindZ.Get(stream), iZ = invertZ.Get(stream);

                Vector3 constrainedEulers, sourceEulers,
                    ao = angleOffset.Get(stream);

                Quaternion constrainedRotation, sourceRotation, destinationRotation;

                if (iL)
                {
                    constrainedRotation = constrained.GetLocalRotation(stream);
                    sourceRotation = source.GetLocalRotation(stream);
                }
                else
                {
                    constrainedRotation = constrained.GetRotation(stream);
                    sourceRotation = source.GetRotation(stream);
                }

                constrainedEulers = constrainedRotation.eulerAngles;
                sourceEulers = sourceRotation.eulerAngles;

                if (bX)
                {
                    if (iX) { constrainedEulers.x = -sourceEulers.x; }
                    else { constrainedEulers.x = sourceEulers.x; }
                }
                if (bY)
                {
                    if (iY) { constrainedEulers.y = -sourceEulers.y; }
                    else { constrainedEulers.y = sourceEulers.y; }
                }
                if (bZ)
                {
                    if (iZ) { constrainedEulers.z = -sourceEulers.z; }
                    else { constrainedEulers.z = sourceEulers.z; }
                }

                destinationRotation = Quaternion.Euler(constrainedEulers + ao + originalAngleOffset);

                if (iL)
                {
                    constrained.SetLocalRotation(stream, math.slerp(
                        constrainedRotation, destinationRotation, w));

                }
                else
                {
                    constrained.SetRotation(stream, math.slerp(
                        constrainedRotation, destinationRotation, w));
                }
            }
        }


    }

    [Serializable]
    public struct OmniRotationData : IAnimationJobData
    {
        public bool maintainOriginalAngleOffset;

        public Transform constrainedObject;
        [SyncSceneToStream] public Transform sourceObject;

        public bool IsValid()
        {
            return !(constrainedObject == null || sourceObject == null);
        }

        public void SetDefaultValues()
        {
            constrainedObject = null;
            sourceObject = null;
        }
    }

    public class OmniRotationBinder : AnimationJobBinder<OmniRotationJob, OmniRotationData>
    {
        public override OmniRotationJob Create(Animator animator, ref OmniRotationData data, Component component)
        {
            Vector3 oao = Vector3.zero;
            if (data.maintainOriginalAngleOffset)
            {
                oao = data.sourceObject.eulerAngles -
                      data.constrainedObject.eulerAngles;
            }

            return new OmniRotationJob()
            {
                constrained = ReadWriteTransformHandle.Bind(animator, data.constrainedObject),
                source = ReadOnlyTransformHandle.Bind(animator, data.sourceObject),
                isLocal = BoolProperty.Bind(animator, component, "isLocal"),
                invertX = BoolProperty.Bind(animator, component, "invertX"),
                invertY = BoolProperty.Bind(animator, component, "invertY"),
                invertZ = BoolProperty.Bind(animator, component, "invertZ"),
                bindX = BoolProperty.Bind(animator, component, "bindX"),
                bindY = BoolProperty.Bind(animator, component, "bindY"),
                bindZ = BoolProperty.Bind(animator, component, "bindZ"),
                angleOffset = Vector3Property.Bind(animator, component, "angleOffset"),
                originalAngleOffset = oao
            };
        }

        public override void Destroy(OmniRotationJob job)
        {

        }
    }
}