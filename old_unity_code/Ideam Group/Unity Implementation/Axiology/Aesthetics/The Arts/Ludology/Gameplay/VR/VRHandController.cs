using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.XR;
using UnityEngine.InputSystem.XR;

using Ideam.Unity.UI.WorldSpace;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

using InputDevice = UnityEngine.XR.InputDevice;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRHandController : VRControl
    {
        
        public enum GrabConnectionMethod
        {
            Parent,
            LockHandTarget,
            DirectSet,
            EqualForces,
            ForceToHand,
            ForceToGrabbed,
            HandLocked
        }
        
        public AnimationControl handControl;
        public Rigidbody physicalHand;
        public TriggerSensor palmSensor;
        public LineRenderer rayRenderer;
        public bool isLeftHanded;

        [Tooltip("When getting a pointing ray from the hand, the ray begins at this point in local space")]
        public Vector3 pointFromPoint;
        public Vector3 PointDirection { get { return poseDriver.transform.forward; } }
        public Ray Point { get { return new Ray(poseDriver.transform.TransformPoint(pointFromPoint), PointDirection); } }

        protected override void RegisterVRDevice(InputDevice device)
        {
            if (device.characteristics.HasFlag(InputDeviceCharacteristics.HeldInHand))
            {
                if (device.characteristics.HasFlag(InputDeviceCharacteristics.Left) && isLeftHanded)
                {
                    inputDevice = device;
                }
                else if (device.characteristics.HasFlag(InputDeviceCharacteristics.Right) && !isLeftHanded)
                {
                    inputDevice = device;
                }
                
            }
            if (raySensor)
            {
                for(int i =0; i < controlRayIDs.Length; i++)
                {
                    raySensor.rays[controlRayIDs[i]].OnStartHitting.sensorEvent += StartPointingAt;
                    raySensor.rays[controlRayIDs[i]].WhileHitting.sensorEvent += WhilePointingAt;
                    raySensor.rays[controlRayIDs[i]].OnStopHitting.sensorEvent += StopPointingAt;
                }
            }
        }

      

        protected override void OnEnable()
        {
            base.OnEnable();

            if (rayRenderer && raySensor)
            {
                Vector3 start = raySensor.rays[controlRayIDs[0]].origin;
                Vector3 end = start + raySensor.rays[controlRayIDs[0]].direction * raySensor.rays[controlRayIDs[0]].length;
                rayRenderer.SetPositions(new Vector3[] { start, end });
            }
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }
        
        #region VRUI Management

        public GameObject currentlyPointingAt;
        public WorldSpaceUIElement pointingAtUI;
        Vector2 currentHitPoint;
        protected virtual void StartPointingAt(RaySensorData sensorData)
        {
            currentlyPointingAt = sensorData.hit.collider.gameObject;
            currentHitPoint = sensorData.hit.textureCoord;
            if (vrui)
            {
                WorldSpaceUIElement hitUI = currentlyPointingAt.GetComponent<WorldSpaceUIElement>();
                if (hitUI)
                {
                    pointingAtUI = hitUI;
                    pointingAtUI.UVPointerEnter(sensorData.hit.textureCoord);
                    vrui.pointingAtUI = pointingAtUI;
                    vrui.uiPointerPos = sensorData.hit.textureCoord;
                    vrui.worldPointerPos = sensorData.hit.point;
                }
                else
                { 
                    vrui.runtimeExplorerTarget = sensorData.hit.collider.gameObject;
                }
            }
        }

        protected virtual void WhilePointingAt(RaySensorData sensorData)
        {
            if(sensorData.hit.collider.gameObject == currentlyPointingAt)
            {
                
                if (pointingAtUI) 
                {
                    pointingAtUI.UVPointerMove(sensorData.hit.textureCoord); 
                }
                if (vrui && vrui.pointingAtUI == pointingAtUI)
                {
                    vrui.uiPointerPos = sensorData.hit.textureCoord;
                    vrui.worldPointerPos = sensorData.hit.point;
                }
               
            }
        }

        protected virtual void StopPointingAt(RaySensorData sensorData)
        {
            if(sensorData.hit.collider.gameObject == currentlyPointingAt)
            {
                if (vrui )
                {
                    if(vrui.runtimeExplorerTarget == currentlyPointingAt)
                    {
                        vrui.runtimeExplorerTarget = null;
                    }
                    
                    if(pointingAtUI == vrui.pointingAtUI) { vrui.pointingAtUI = null; }
                }

                if (pointingAtUI)
                {
                    pointingAtUI.UVPointerExit(sensorData.hit.textureCoord);
                    pointingAtUI = null;
                }
                currentlyPointingAt = null;
            }
        }

        #endregion

        #region Haptics

        public void CollisionHaptics()
        {
        }

        #endregion

        public static VRHandController NewHandController(Transform parent = null, bool isLeftHand = true, RaySensor raySensor = null)
        {
            
            GameObject handObject = new GameObject();
            if (isLeftHand) { handObject.name = "Left Hand Controller"; }
            else { handObject.name = "Right Hand Controller"; }

            if(parent != null) handObject.transform.SetParent(parent, false);

            VRHandController handController = AddHandController(handObject, parent, isLeftHand, raySensor);
            return handController;
        }

        public static VRHandController AddHandController(GameObject handObject, Transform controlRoot = null, bool isLeftHand = true, RaySensor raySensor = null)
        {
            VRHandController handController = handObject.AddComponent<VRHandController>();
            handController.isLeftHanded = isLeftHand;

            handController.controlRoot = controlRoot;
            handController.poseDriver = handObject.AddComponent<TrackedPoseDriver>();
            
            handController.poseDriver.trackingType = TrackedPoseDriver.TrackingType.RotationAndPosition;

            handController.physicalHand = handObject.AddComponent<Rigidbody>();
            handController.physicalHand.isKinematic = true;
            handController.physicalHand.useGravity = false;
            handController.physicalHand.constraints = RigidbodyConstraints.FreezeAll;
            handController.physicalHand.mass = 0.0001f;
            handController.physicalHand.angularDrag = 0f;
            handController.physicalHand.interpolation = RigidbodyInterpolation.Interpolate;
            handController.physicalHand.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;

            GameObject palmSensorObject = new GameObject();
            if (isLeftHand) { palmSensorObject.name = "Left Palm Sensor"; }
            else { palmSensorObject.name = "Right Palm Sensor"; }
            palmSensorObject.transform.SetParent(handController.transform);
            
            handController.palmSensor = palmSensorObject.AddComponent<TriggerSensor>();
            handController.palmSensor.IsSensing = true;

            SphereCollider palmTrigger = palmSensorObject.AddComponent<SphereCollider>();
            palmTrigger.isTrigger = true;
            palmTrigger.radius = 0.02f;
            if (isLeftHand) { palmTrigger.center = new Vector3(0.01f, 0, 0); } 
            else { palmTrigger.center = new Vector3(-0.01f, 0, 0); }

            if (raySensor)
            {
                handController.raySensor = raySensor;
                handController.controlRayIDs = new int[1];

                int l = raySensor.rays.Length;
                handController.controlRayIDs[0] = l;
                System.Array.Resize(ref raySensor.rays, l + 1);

                SensorRay handRay = new SensorRay();
                if (isLeftHand) { handRay.name = "Left Hand Pointer"; } 
                else { handRay.name = "Right Hand Pointer"; }
                handRay.origin = handController.pointFromPoint;
                handRay.length = 20f;
                handRay.originSpace = Space.Self;
                handRay.originTransform = handController.transform;
                handRay.direction = Vector3.forward;
                handRay.directionSpace = Space.Self;

                raySensor.rays[l] = handRay;
            }
            return handController;
        }
    }
        
}
