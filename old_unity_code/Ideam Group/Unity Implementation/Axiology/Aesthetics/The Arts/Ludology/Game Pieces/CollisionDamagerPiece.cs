using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay
{
    public class CollisionDamagerPiece : UnityGamePiece
    {
        public CollisionDamageDealer damageDealer;
        public int[] damageSourceActionIDs;

        public int[] targetResourceIDs;

        private void Awake()
        {
            for(int i = 0; i < damageDealer.damageSources.Length; i++)
            {
                int id = i;
                damageDealer.damageSources[i].OnDamage += (ob, val) => { DoDamage(id, ob, val); };
            }
        }

        private void DoDamage(int id, GameObject ob, float val)
        {
            UnityGamePiece otherPiece = ob.GetComponent<UnityGamePiece>();
            if (otherPiece)
            {
                UnityGameAgent otherAgent = otherPiece.gameAgent;
                if(otherPiece is CollisionDamagerPiece otherDamager)
                {

                }

                if (gameAgent && otherAgent)
                {
                    /*
                    UnityGameInteraction interaction = new UnityGameInteraction();
                    interaction.interactionType = UnityGameInteraction.InteractionType.Singular;

                    interaction.causalEntities.Add(gameAgent);
                    
                    interaction.causalActions.Add(gameAgent.actions[damageSourceActionIDs[id]]);

                    otherAgent.Interact(interaction);
                    */
                }
                
                int damageValue = Mathf.FloorToInt(val);

                if (gameAgent)
                {
                    gameAgent.StartAction(damageSourceActionIDs[id]);
                    //Now modify it with action value.
                    gameAgent.UpdateAction(damageSourceActionIDs[id], val);
                    damageValue = gameAgent.actions[id].CurrentActionValue;
                }

                int otherToDamage = otherPiece.GetLocalResourceID(targetResourceIDs[id]);
                if (otherToDamage >=0)
                {
                    otherPiece.ExhaustResource(otherToDamage, damageValue);
                }

                gameAgent.CompleteAction(damageSourceActionIDs[id]);
            }
        }
    }
}