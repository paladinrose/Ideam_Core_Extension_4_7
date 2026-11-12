using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class TurnControl : BaseControl
    {
        public float turnSpeed, maxAngularVelocityChange;
       

        protected int setTurnCounter;
        protected Vector3 setTurn, averagedTurn;
       

        private void Awake()
        {
            GetAnimatedPropertyIDs();
        }

        public void Turn(Vector3 turnBy, Space turnSpace = Space.Self)
        {
            if(turnSpace == Space.World) { turnBy = controlTarget.InverseTransformDirection(turnBy); }
            setTurn += turnBy;
            setTurnCounter++;
        }

        public void TurnTo(Vector3 angle, Space turnSpace = Space.Self)
        {
            angle = (angle - controlTarget.eulerAngles);
            if(turnSpace == Space.World) { angle = controlTarget.InverseTransformDirection(angle); }
            setTurn += angle;
            setTurnCounter++;
        }

        public void UpdateTurnControl()
        {
            if (controlFlags == ControlFlags.Disabled || setTurnCounter == 0) { setTurn = Vector3.zero; setTurnCounter = 0; return; }

            averagedTurn = setTurn / setTurnCounter;

            if (controlFlags.HasFlag(ControlFlags.Animated)) 
            {
                AnimatedProperty ap = animatedProperties[0];
                ap.value = averagedTurn.x;
                animatedProperties[0] = ap;

                ap = animatedProperties[1];
                ap.value = averagedTurn.y;
                animatedProperties[1] = ap;

                ap = animatedProperties[2];
                ap.value = averagedTurn.z;
                animatedProperties[2] = ap;

                UpdateAnimatedProperties();
            }

            if (controlFlags.HasFlag(ControlFlags.Physical)) { UpdateTurnPhysics(); }
            else if (controlFlags.HasFlag(ControlFlags.Direct)) { UpdateTurnDirect(); }

            setTurn = Vector3.zero; setTurnCounter = 0;
        }

        private void UpdateTurnPhysics()
        {
            if (controlFlags.HasFlag(ControlFlags.Kinematic))
            {
                Vector3 worldTurn = controlTarget.TransformDirection(averagedTurn);
                controlBody.MoveRotation(controlTarget.rotation * Quaternion.Euler(worldTurn.normalized * turnSpeed));
            }
            else
            {
                var velocity = controlTarget.InverseTransformDirection(controlBody.angularVelocity);

                var targetVelocity = averagedTurn.normalized * turnSpeed;
                
                if (maxAngularVelocityChange > 0)
                {
                    targetVelocity.x = Mathf.Clamp(targetVelocity.x, -maxAngularVelocityChange, maxAngularVelocityChange);
                    targetVelocity.z = Mathf.Clamp(targetVelocity.z, -maxAngularVelocityChange, maxAngularVelocityChange);
                    targetVelocity.y = Mathf.Clamp(targetVelocity.y, -maxAngularVelocityChange, maxAngularVelocityChange);
                }

                var velocityChange = (targetVelocity - velocity);
               
                controlBody.AddRelativeTorque(velocityChange, ForceMode.VelocityChange);
            }
        }

        private void UpdateTurnDirect()
        {
            controlTarget.eulerAngles += averagedTurn.normalized * turnSpeed * Time.fixedDeltaTime;
        }
    }
}