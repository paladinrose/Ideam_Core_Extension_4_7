using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    //Later on, this needs to becomes a "HandedAgent", and "Humanoid" can descend from that. For now,
    //it'll do.
    public class HumanoidAgent : TwoHandedAgent
    {
        public HumanoidAnimationRigging humanoidRig;
        //Things to account for: Walking, running, jumping, hand and feet IK (ie for climbing)
        
        protected bool isRunning;
        
        //Running: Agent is moving. Character presses run. Move uses their Run action to determine their
        //speed.
        
        //Jumping: Agent is grounded or can air jump. Check grounded using mind>senses. If not, and there
        //IS an air jump action, perform that action.

        //Crouching: Agent is grounded. If moving, agent uses CrouchMove and/or CrouchRun actions to
        //determine their speed, etc.

        protected override void Awake()
        {
           
        }


        public virtual void Crouch()
        {

        }

        public virtual void Sit()
        {

        }
        public virtual void Stand()
        {

        }

        
        public virtual void LayDown() { }

    }
}