using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{ 
    public class MoveControl : BaseControl
    {
        public event Action<float> OnSpeedChange, OnMaxVelocityChange;
        public event Action OnStartMoving, OnStopMoving;
        public event Action<Vector3> OnMove;
        
        [SerializeField]
        protected float speed, maxVelocityChange;
        public float Speed { get { return speed; } set { if (speed == value) return; speed = value; OnSpeedChange?.Invoke(speed); } }
        public float MaxVelocityChange { get { return maxVelocityChange; } set { if (maxVelocityChange == value) return; maxVelocityChange = value; OnMaxVelocityChange?.Invoke(maxVelocityChange); } }
        
        protected int setMoveCounter;
        protected Vector3 setMove, averagedMove;
        protected bool doStartMove, doStopMove;

        private void Awake()
        {
            GetAnimatedPropertyIDs();
        }

        public void Move(Vector3 moveBy, Space moveSpace = Space.Self)
        {
            if(moveSpace == Space.World) { moveBy = controlTarget.InverseTransformDirection(moveBy); }
           
            doStopMove = false;

            setMove += moveBy;
            
            if(setMoveCounter == 0) { doStartMove = true; }
            
            setMoveCounter++;
        }

        public void MoveTo(Vector3 position, Space moveSpace = Space.Self)
        {
            position = (position - controlTarget.position);
            
            if(moveSpace == Space.World) { position = controlTarget.InverseTransformDirection(position); }
            
            setMove += position;

            doStopMove = false;

            if (setMoveCounter == 0) { doStartMove = true; }

            setMoveCounter++;
        }

        public void UpdateMoveControl()
        {
            if(controlFlags == ControlFlags.Disabled || setMoveCounter == 0) 
            {
                setMove = Vector3.zero; 
                setMoveCounter = 0;
                if (doStopMove)
                {
                    OnStopMoving?.Invoke();
                    doStopMove = false;
                }
                return; 
            }

            if (doStartMove) { OnStartMoving?.Invoke(); doStartMove = false; }

            averagedMove = setMove / setMoveCounter;

            if (controlFlags.HasFlag(ControlFlags.Animated)) 
            {
                AnimatedProperty ap = animatedProperties[0];
                ap.value = averagedMove.x;
                animatedProperties[0] = ap;

                ap = animatedProperties[1];
                ap.value = averagedMove.y;
                animatedProperties[1] = ap;

                ap = animatedProperties[2];
                ap.value = averagedMove.z;
                animatedProperties[2] = ap;

                UpdateAnimatedProperties();
            }

            if(controlFlags.HasFlag(ControlFlags.Physical)) { UpdateMovePhysics(); }
            else if (controlFlags.HasFlag(ControlFlags.Direct)) { UpdateMoveDirect(); }

            OnMove?.Invoke(averagedMove);

            setMove = Vector3.zero; 
            setMoveCounter = 0;
            doStopMove = true;
        }


        private void UpdateMovePhysics()
        {
            if (controlFlags.HasFlag(ControlFlags.Kinematic))
            {
                Vector3 worldMove = controlTarget.TransformDirection(averagedMove);
                controlBody.MovePosition(controlTarget.position + (worldMove.normalized*speed));
            }
            else
            {
                Vector3 moveAtSpeed = averagedMove.normalized*speed;
                
                var velocity = controlTarget.InverseTransformDirection(controlBody.velocity);
                
                var targetVelocity = moveAtSpeed;

                //Replace later with my own gravity controls.
                targetVelocity += controlTarget.InverseTransformDirection(UnityEngine.Physics.gravity * Time.fixedDeltaTime);
                
                if (maxVelocityChange > 0)
                {
                    targetVelocity.x = Mathf.Clamp(targetVelocity.x, -maxVelocityChange, maxVelocityChange);
                    targetVelocity.z = Mathf.Clamp(targetVelocity.z, -maxVelocityChange, maxVelocityChange);
                    targetVelocity.y = Mathf.Clamp(targetVelocity.y, -maxVelocityChange, maxVelocityChange);
                }

                var velocityChange = (targetVelocity - velocity);
               
                controlBody.AddRelativeForce(velocityChange, ForceMode.VelocityChange);
            }
        }


        private void UpdateMoveDirect()
        {
            controlTarget.position += controlTarget.TransformDirection(averagedMove.normalized * speed * Time.fixedDeltaTime);
        }
        
    }
}