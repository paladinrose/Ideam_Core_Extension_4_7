using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.XR;
using UnityEngine.Rendering;
using UnityEngine.Audio;
using UnityEngine.UI;
using UnityEngine.InputSystem.XR;

using Cinemachine;
using Cinemachine.PostFX;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Unity.UI.WorldSpace;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRHeadController : VRControl
    {
        //We can change the look and sound of the player's experience by changing the camera settings,
        //the post effects, scriptable renderer settings, audio mixers, and audio filters.

        public virtual event Action WhenHeadIsReady;

        public AnimationControl headControl;

        [Header("Visuals")]
        public Camera physicalCamera;
        public Camera[] physicalStack;
        public CinemachineVirtualCamera baseCamera;
        public CinemachineVolumeSettings baseVolumeSettings;
        public CinemachineVirtualCamera[] alternateCameras;
        public CinemachineVolumeSettings[] alternateVolumeSettings;

        public VolumeProfile[] volumeProfiles;

        
        public RectTransform leftEyeOverlay, rightEyeOverlay, bothEyesOverlay;
        public WorldSpaceUserInterface leftEyeUI, rightEyeUI, bothEyesUI;
        public RenderTexture leftEyeRenderTexture, rightEyeRenderTexture, bothEyesRenderTexture;
        public Material leftEyeMaterial, rightEyeMaterial, bothEyesMaterial;

        //TODO: I want to make two more standard rays: Left Eye Convergence and Right Eye Convergence.
        // After Focus fires and positions its target, FindConvergence() is called. It points the local direction
        // of both rays at that focus point, then fires them.

        // Why? Well, in theory, if I position, say, a UI element in Left Eye Overlay that's colored red,
        // and another on Right Eye Overlay that's colored green,
        // and they're both positioned on their respective rays of convergence at the same distance,
        // the player 'should' see the element in the correct 3d space, colored yellow.
        
        
        public Transform focusTarget;

        public TriggerSensor peripheralVision;

        int leftBlinkPropertyID, rightBlinkPropertyID;
        Coroutine blinkRoutine;

        [Header("Audio")]
        public AudioMixer audioMixer;
        public AudioListener audioListener;

        public AudioChorusFilter[] chorusFilters;
        public AudioDistortionFilter[] distortionFilters;
        public AudioEchoFilter[] echoFilters;
        public AudioHighPassFilter[] highPassFilters;
        public AudioLowPassFilter[] lowPassFilters;
        public AudioReverbFilter[] reverbFilters;

        protected float baseVRHeight;
        public float BaseVRHeight { get { return baseVRHeight; } }

        protected Vector3 vrPositionOffset;
        public Vector3 VRPositionOffset { get { return vrPositionOffset; } }

        protected bool headIsReady;
        public bool HeadIsReady { get { return headIsReady; } }

        public Vector3 GazeDirection { get { return poseDriver.transform.forward; } }
        public Ray Gaze { get { return new Ray(poseDriver.transform.position, GazeDirection); } }

        protected override void RegisterVRDevice(InputDevice device)
        {
            if (device.characteristics.HasFlag(InputDeviceCharacteristics.HeadMounted))
            {
                StartCoroutine(RegisterHeadHeight());
            }
            
        }

        protected virtual IEnumerator RegisterHeadHeight()
        {
            //We start by waiting for a second: the hmd is still loading and this gives the game world
            //a chance to settle down before we measure our height.
            yield return new WaitForSeconds(1f);

            Transform root = controlRoot;
            Vector3 localizedHeadPos = root.InverseTransformPoint(transform.position);
            localizedHeadPos = new Vector3(0, localizedHeadPos.y, 0);

            Vector3 rootHeadsUp = root.TransformDirection(localizedHeadPos);
            baseVRHeight = rootHeadsUp.magnitude;
            vrPositionOffset = (transform.position * -1) + rootHeadsUp;

            float frustumHeight = 2.0f * bothEyesOverlay.localPosition.z * Mathf.Tan(physicalCamera.fieldOfView * 0.5f * Mathf.Deg2Rad);
            float frustumWidth = frustumHeight * physicalCamera.aspect;
            
            
            bothEyesOverlay.sizeDelta = new Vector2(frustumWidth, frustumHeight);
            

            if(leftEyeMaterial) leftBlinkPropertyID = leftEyeMaterial.shader.FindPropertyIndex("_Blink");
            if(rightEyeMaterial) rightBlinkPropertyID = rightEyeMaterial.shader.FindPropertyIndex("_Blink");

            if (raySensor)
            {
                raySensor.rays[controlRayIDs[0]].WhileHitting.sensorEvent += FocusTarget;

                for (int i = 1; i < controlRayIDs.Length; i++)
                {
                    int id = controlRayIDs[i];
                    raySensor.rays[id].OnStartHitting.sensorEvent += OnFocusStart;
                    raySensor.rays[id].OnStopHitting.sensorEvent += OnFocusStop;
                }
            }

            if (peripheralVision)
            {
                peripheralVision.OnTriggerStart.sensorEvent += OnEnterPeripheralVision;
                peripheralVision.OnTriggerStart.sensorEvent += OnExitPeripheralVision;
            }
            headIsReady = true;

            WhenHeadIsReady?.Invoke();
        }

        #region Vision UI

        private void OnFocusStart(RaySensorData sensorData)
        {
            UnityGamePiece pieceHit = sensorData.hit.collider.gameObject.GetComponent<UnityGamePiece>();
            if (pieceHit)
            {
                UnityGameAgent agentHit = pieceHit.gameAgent;

                if (vrui)
                {
                    if (agentHit)
                    {
                        vrui.ExploreAgent(agentHit);
                    }
                    else
                    {
                        vrui.ExploreGamePiece(pieceHit);
                    }
                }
            }
        }

        private void OnFocusStop(RaySensorData sensorData)
        {
           
        }

        private void OnEnterPeripheralVision(TriggerSensorData sensorData)
        {

        }

        private void OnExitPeripheralVision(TriggerSensorData sensorData)
        {
            UnityGamePiece pieceHit = sensorData.collider.gameObject.GetComponent<UnityGamePiece>();
            if (pieceHit)
            {
                UnityGameAgent agentHit = pieceHit.gameAgent;

                if (vrui)
                {
                    if (agentHit)
                    {
                        vrui.StopExploringAgent(agentHit);
                    }
                    else
                    {
                        vrui.StopExploringGamePiece(pieceHit);
                    }
                }
            }
        }

        private void FocusTarget(RaySensorData sensorData)
        {
            focusTarget.position = sensorData.hit.point;
        }

        #endregion


        public void CloseEyes()
        {
            if (blinkRoutine != null)
            {
                StopCoroutine(blinkRoutine);
            }
            Spline1 eyeSpline = Spline1.ZeroToOne();
            Eyelids(eyeSpline, eyeSpline, 0.1f);
        }

        public void OpenEyes()
        {
            if (blinkRoutine != null)
            {
                StopCoroutine(blinkRoutine);
            }
            Spline1 eyeSpline = Spline1.OneToZero();
            Eyelids(eyeSpline, eyeSpline, 0.1f);
        }

        public void BlinkEyes()
        {
            if (blinkRoutine != null)
            {
                StopCoroutine(blinkRoutine);
            }
            Spline1 eyeSpline = new Spline1(3);
            eyeSpline[0] = 0f;
            eyeSpline.sections[0].weight = 0.75f;
            eyeSpline[1] = 1f;
            eyeSpline.sections[1].weight = 0.25f;
            eyeSpline[2] = 0f;
            
            Eyelids(eyeSpline, eyeSpline, 0.1f);
        }

        IEnumerator Eyelids(Spline1 leftBlinkCurve, Spline1 rightBlinkCurve, float time)
        {
            float leftValue = 0, rightValue = 0;
            float currentTime = 0, timeScalar = 1f/time;

            while(currentTime < time)
            {
                if(leftEyeMaterial) leftEyeMaterial.SetFloat(leftBlinkPropertyID, leftValue);
                if(rightEyeMaterial) rightEyeMaterial.SetFloat(rightBlinkPropertyID, rightValue);

                currentTime += Time.deltaTime;
                float p = currentTime * timeScalar;
                leftValue = leftBlinkCurve.GetPoint(p);
                rightValue = rightBlinkCurve.GetPoint(p);

                yield return null;
            }

            leftValue = leftBlinkCurve.GetPoint(1f);
            rightValue = rightBlinkCurve.GetPoint(1f);

            if(leftEyeMaterial) leftEyeMaterial.SetFloat(leftBlinkPropertyID, leftValue);
            if(rightEyeMaterial) rightEyeMaterial.SetFloat(rightBlinkPropertyID, rightValue);
           
            
            yield break;
        }

        public static VRHeadController NewVRHeadController(Transform parent = null, RaySensor raySensor = null)
        {
            GameObject headObject = new GameObject("Head Controller");
            if(parent != null) headObject.transform.SetParent(parent);
            headObject.transform.localPosition = headObject.transform.localEulerAngles = Vector3.zero;


            VRHeadController headController = AddVRHeadController(headObject, parent, raySensor);

            return headController;
        }

        public static VRHeadController AddVRHeadController(GameObject headObject, Transform controlRoot = null, RaySensor raySensor = null)
        {
            VRHeadController headController = headObject.AddComponent<VRHeadController>();

            headController.physicalCamera = Camera.main;
            headController.baseCamera = headObject.AddComponent<CinemachineVirtualCamera>();
            headController.baseCamera.m_Lens.NearClipPlane = 0.01f;
            headController.controlRoot = controlRoot;

            headController.poseDriver = headObject.AddComponent<TrackedPoseDriver>();
            headController.poseDriver.trackingType = TrackedPoseDriver.TrackingType.RotationAndPosition;
            

            headController.baseVolumeSettings = headObject.AddComponent<Cinemachine.PostFX.CinemachineVolumeSettings>();
            headController.baseCamera.AddExtension(headController.baseVolumeSettings);

            GameObject focalPoint = new GameObject("Focal Point");
            focalPoint.transform.SetParent(headController.transform, false);
            focalPoint.transform.localPosition = focalPoint.transform.localEulerAngles = Vector3.zero;
            headController.focusTarget = focalPoint.transform;

            GameObject beo = new GameObject("Eyes Overlay", typeof(RectTransform));
            headController.bothEyesOverlay = beo.GetComponent<RectTransform>();
            headController.bothEyesOverlay.SetParent(headController.transform);
            headController.bothEyesUI = headController.bothEyesOverlay.gameObject.AddComponent<WorldSpaceUserInterface>();
            headController.bothEyesOverlay.sizeDelta = new Vector2(2f, 1f);

            GameObject leo = new GameObject("Left Eye Overlay", typeof(RectTransform));
            headController.leftEyeOverlay = leo.GetComponent<RectTransform>();
            headController.leftEyeOverlay.SetParent(headController.bothEyesOverlay);
            headController.leftEyeUI = headController.leftEyeOverlay.gameObject.AddComponent<WorldSpaceUserInterface>();
            headController.leftEyeOverlay.anchorMin = new Vector2(0, 0);
            headController.leftEyeOverlay.anchorMax = new Vector2(0.5f, 1f);
            headController.leftEyeOverlay.sizeDelta = Vector2.zero;

            GameObject reo = new GameObject("Right Eye Overlay", typeof(RectTransform));
            headController.rightEyeOverlay = reo.GetComponent<RectTransform>();
            headController.rightEyeOverlay.SetParent(headController.bothEyesOverlay);
            headController.rightEyeUI = headController.rightEyeOverlay.gameObject.AddComponent<WorldSpaceUserInterface>();
            headController.rightEyeOverlay.anchorMin = new Vector2(0.5f, 0);
            headController.rightEyeOverlay.anchorMax = new Vector2(1f, 1f);
            headController.rightEyeOverlay.sizeDelta = Vector2.zero;


            headController.baseVolumeSettings.m_FocusTracking = Cinemachine.PostFX.CinemachineVolumeSettings.FocusTrackingMode.CustomTarget;
            headController.baseVolumeSettings.m_FocusTarget = headController.focusTarget;


            if (raySensor)
            {
                headController.raySensor = raySensor;
                headController.controlRayIDs = new int[1];

                int l = raySensor.rays.Length;
                headController.controlRayIDs[0] = l;
                System.Array.Resize(ref raySensor.rays, l + 1);

                SensorRay headRay = new SensorRay();
                headRay.name = "Line of Sight";
                headRay.length =  500f;
                headRay.origin = Vector3.forward*0.003f;
                headRay.originSpace = Space.Self;
                headRay.originTransform = headController.transform;
                headRay.direction = Vector3.forward;
                headRay.directionSpace = Space.Self;

                raySensor.rays[l] = headRay;
            }

            return headController;
        }
    }   
}
