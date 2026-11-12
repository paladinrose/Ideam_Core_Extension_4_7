using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Metaphysics.Ontology.Action;
using System;
using Ideam.Axiology.Valuation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay
{
    public class ActionableCollisionDamageDealer : CollisionDamageDealer, IActionable
    {
        public Action<T>[] GetContinuousAndRepeatedActions<T>() where T : IValue<T>
        {
            return new Action<T>[] 
            {
                t => {}
            };
        }

        public Action<T>[] GetInitiatedActions<T>() where T : IValue<T>
        {
            int l = damageSources.Length;
            Action<T>[] actionInitiators = new Action<T>[l];
           
            for(int i =0; i < l; i++)
            {
                Action<T> damageInitiator = t => { };
                damageSources[i].OnDamage += (ob, od) =>
                {
                    T tod = default;
                    if (tod.TryConvert(od, out tod))
                    { 
                        damageInitiator.Invoke(tod);
                    }
                };
                actionInitiators[i] = damageInitiator;
            }

            return actionInitiators;
        }
    }
}