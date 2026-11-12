using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class PushControl : BaseControl
    {
        [System.Serializable]
        public struct PushData
        {
            public bool requirePushTarget;
            public Transform pusher, pushTarget;
            public Rigidbody pusherBody, pushBody;
            public Vector3 pushFrom;
            public Vector3 pushForce;
            public int pushCount;
            public CollisionSensor collisionSensor;
            
            public PushData(PushData pushData) 
            {
                requirePushTarget = pushData.requirePushTarget;
                pusher = pushData.pusher;
                pushTarget = pushData.pushTarget;
                pusherBody = pushData.pusherBody;
                pushBody = pushData.pushBody;
                pushFrom = pushData.pushFrom;
                pushForce = pushData.pushForce;
                pushCount = pushData.pushCount;
                collisionSensor = pushData.collisionSensor;
            }
        }


        public PushData[] pushData;

        public void Push(PushData push, Space pushSpace = Space.Self) 
        {
            if(push.requirePushTarget && push.pushTarget == null) { return; }

            int cpID = -1, firstEmpty = -1;
            for(int i = 0; i < pushData.Length; i++)
            {
                if(pushData[i].pushTarget == push.pushTarget)
                {
                    cpID = i;
                    break;
                }
                else if(firstEmpty < 0 && pushData[i].pushTarget == null)
                {
                    firstEmpty = i;
                }
            }

            if(cpID >= 0)
            {

            }
            else if(firstEmpty > 0)
            {

            }
        }

        public void PushUpdate()
        {

        }
    }
}