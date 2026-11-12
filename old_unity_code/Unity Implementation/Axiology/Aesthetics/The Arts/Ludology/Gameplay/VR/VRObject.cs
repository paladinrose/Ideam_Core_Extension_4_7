using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.Events;
using UnityEngine;
using UnityEngine.InputSystem;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRObject : UnityGamePiece
    {
        [Serializable]
        public struct VRObjectUse
        {
            public int agentActionID;
            public VRCommonActions useMap;
            public UnityEvent[] useEvents;
            public void Use()
            {
                for(int i =0; i < useEvents.Length; i++)
                {
                    useEvents[i].Invoke();
                }
            }
        }
        public VRHandController primaryHand;
        public VRObjectUse[] objectUses;
        
        public Vector3 localGripPositionOffset;
        
        public Vector3 localGripAngleOffset;
        public bool flipLocalXForLeftHand;

        public Vector3 GripPosition { get { 
                if (primaryHand && primaryHand.isLeftHanded && flipLocalXForLeftHand) 
                {
                    Vector3 flipped = localGripPositionOffset;
                    flipped.x *= -1;
                    return transform.TransformPoint(flipped);
                }
                else
                {
                    return transform.TransformPoint(localGripPositionOffset);
                }
            } }
        public Quaternion GripRotation { get { return transform.rotation * Quaternion.Euler(localGripAngleOffset); } }
            

        void OnDrawGizmosSelected()
        {
            Vector3 gp = GripPosition;
            Gizmos.DrawWireCube(gp, Vector3.one * 0.01f);

            
            Matrix4x4 rotMat = Matrix4x4.TRS(gp, GripRotation, transform.localScale);
            
            Vector3 eX = rotMat.MultiplyPoint3x4(Vector3.right),
                    eY = rotMat.MultiplyPoint3x4(Vector3.up),
                    eZ = rotMat.MultiplyPoint3x4(Vector3.forward);
            
            Gizmos.color = Color.red;
            Gizmos.DrawLine(gp, eX);

            Gizmos.color = Color.green;
            Gizmos.DrawLine(gp, eY);

            Gizmos.color = Color.blue;
            Gizmos.DrawLine(gp, eZ);
        }
    }

    
}