using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay
{
    public class PickupPiece : UnityGamePiece
    {
        public CollisionSensor collisionSensor;

        //As in other places, eventually I want to update this to using a pure Semantics based approach. But that's for later.
        public string pickupAction;

        private void OnEnable()
        {
            collisionSensor.OnCollisionStart.sensorEvent += CheckForPickup;
        }
        
        private void OnDisable()
        {
            collisionSensor.OnCollisionStart.sensorEvent += CheckForPickup;
        }

        void CheckForPickup(CollisionSensorData sensorData)
        {
            UnityGameAgent agent = sensorData.collision.gameObject.GetComponent<UnityGameAgent>();
            UnityGameInteraction pickupInteraction = null;
            if (agent)
            {
                pickupInteraction = PickupInteraction(agent);

            }
            else
            {
                UnityGamePiece collidingPiece = sensorData.collision.gameObject.GetComponent<UnityGamePiece>();
                if(collidingPiece && collidingPiece.gameAgent)
                {
                    pickupInteraction = PickupInteraction(collidingPiece.gameAgent);
                }
            }

            if (pickupInteraction != null)
            {

            }
        }

        UnityGameInteraction PickupInteraction(UnityGameAgent agent)
        {
            int agentActionID = agent.GetActionID(pickupAction);
            UnityGameAction agentAction = agent.actions[agentActionID];
            UnityGameInteraction pickupInteraction = new UnityGameInteraction() 
            {
                causalAgents = new UnityGameAgent[] {agent},
                causalActions = new UnityGameAction[] {agentAction},
                causalPieces = new UnityGamePiece[] {this},

                effectedAgents = new UnityGameAgent[] {agent},
 
            };

            return pickupInteraction;
        }
        
    }
}