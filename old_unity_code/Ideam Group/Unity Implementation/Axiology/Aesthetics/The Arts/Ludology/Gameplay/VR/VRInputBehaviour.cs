using System;

using UnityEngine;
using UnityEngine.InputSystem;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRInputBehaviour: MonoBehaviour
    {
		

		protected virtual event Action OnFixedUpdate;
		protected virtual void FixedUpdate()
		{
			OnFixedUpdate?.Invoke();
		}

		
	}
}