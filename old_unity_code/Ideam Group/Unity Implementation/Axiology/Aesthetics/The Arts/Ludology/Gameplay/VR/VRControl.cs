using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.InputSystem.XR;
//using UnityEngine.SpatialTracking;
using UnityEngine.XR;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

using InputDevice = UnityEngine.XR.InputDevice;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{

    public enum VRCommonActions
	{
		None,
		HeadGaze,

		PointEither,
		TriggerEither,
		GripEither,
		StickEither,
		StickButtonEither,
		PrimaryButtonEither,
		SecondaryButtonEither,
		MenuButtonEither,

		PointLeft,
		TriggerLeft,
		GripLeft,
		StickLeft,
		StickButtonLeft,
		PrimaryButtonLeft,
		SecondaryButtonLeft,
		MenuButtonLeft,

		PointRight,
		TriggerRight,
		GripRight,
		StickRight,
		StickButtonRight,
		PrimaryButtonRight,
		SecondaryButtonRight,
		MenuButtonRight
	}

	public class VRControl : MonoBehaviour
    {

		[Tooltip("The root of the VR control that accounts for the real-world " +
			"space in which the player is moving. It's used for offsetting the players' height, etc...")]
		public Transform controlRoot;
		
		[Tooltip("The input driver that tracks the position and rotation information of the physical " +
			"control and translates them into game world space.")]
		public TrackedPoseDriver poseDriver;

		public InputDevice inputDevice;

		public VRUI vrui;

		public RaySensor raySensor;
		public int[] controlRayIDs;
		protected virtual void RegisterVRDevice(InputDevice device)
        {

        }

		protected virtual void DeviceLost(InputDevice device)
        {
			if(device != inputDevice) { return; }
        }
		protected virtual void DeviceConfigChange(InputDevice device)
        {
			if(device != inputDevice) { return; }
        }

		protected virtual void LinkToDeviceConnectionEvents()
        {
			InputDevices.deviceConnected += RegisterVRDevice;
			InputDevices.deviceDisconnected += DeviceLost;
			InputDevices.deviceConfigChanged += DeviceConfigChange;
		
        }

		protected virtual void UnlinkFromDeviceConnectionEvents()
        {
			InputDevices.deviceConnected -= RegisterVRDevice;
			InputDevices.deviceDisconnected -= DeviceLost;
			InputDevices.deviceConfigChanged -= DeviceConfigChange;
		}

		protected virtual void OnEnable()
        {
			LinkToDeviceConnectionEvents();
        }
		protected virtual void OnDisable() 
		{
			UnlinkFromDeviceConnectionEvents();
		}
		protected virtual void OnDestroy()
        {
			UnlinkFromDeviceConnectionEvents();
        }
    }
}