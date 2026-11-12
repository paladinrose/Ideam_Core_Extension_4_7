using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Editor;
using Ideam.Unity.UI;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Science.Natural.Physics
{
    [CustomEditor(typeof(PhysicalAnimator))]
    public class PhysicalAnimatorEditor : UIElementsEditor
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
            "isSetup", "physicsBones", "groups", "physicsEnabled", "totalMass", "averageDensity",
            "solverIterations", "solverVelocityIterations", "maxAngularVelocity"
            }).ToArray();
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            physicalAnimator = (PhysicalAnimator)target;
        }

        PhysicalAnimator physicalAnimator;
        Toggle enablePhysics;
        VisualElement selectedBoneField;
        DropList physicsBonesList;
        IntegerField solverItsField, solverVelocityItsField;
        FloatField massField, densityField, maxAngularVelocityField;
        
        IEnumerator skeletonRoutine;

        public override VisualElement CreateInspectorGUI()
        {
            VisualElement baseElement = base.CreateInspectorGUI();

            enablePhysics = new Toggle("Physics Enabled");
            enablePhysics.value = physicalAnimator.PhysicsEnabled;
            enablePhysics.RegisterValueChangedCallback(evt => { physicalAnimator.PhysicsEnabled = evt.newValue; });
            baseElement.Add(enablePhysics);

            massField = new FloatField("Total Mass");
            massField.value = physicalAnimator.TotalMass;
            massField.isDelayed = true;

            baseElement.Add(massField);

            densityField = new FloatField("Average Density");
            densityField.value = physicalAnimator.AverageDensity;
            densityField.isDelayed = true;
            densityField.RegisterValueChangedCallback(evt => { physicalAnimator.AverageDensity = evt.newValue; massField.SetValueWithoutNotify(physicalAnimator.TotalMass); SetDollAndPartsDirty(); });
            baseElement.Add(densityField);
            massField.RegisterValueChangedCallback(evt => { physicalAnimator.TotalMass = evt.newValue; densityField.SetValueWithoutNotify(physicalAnimator.AverageDensity); SetDollAndPartsDirty(); });


            solverItsField = new IntegerField("Solver Iterations");
            solverItsField.value = physicalAnimator.SolverIterations;
            solverItsField.isDelayed = true;
            solverItsField.RegisterValueChangedCallback(evt =>
            {
                physicalAnimator.SolverIterations = evt.newValue;
                SetDollAndPartsDirty();
            });
            baseElement.Add(solverItsField);

            solverVelocityItsField = new IntegerField("Solver Velocity Iterations");
            solverVelocityItsField.value = physicalAnimator.SolverVelocityIterations;
            solverVelocityItsField.isDelayed = true;
            solverVelocityItsField.RegisterValueChangedCallback(evt =>
            {
                physicalAnimator.SolverVelocityIterations = evt.newValue;
                SetDollAndPartsDirty();
            });
            baseElement.Add(solverVelocityItsField);

            maxAngularVelocityField = new FloatField("Max Angular Velocity");
            maxAngularVelocityField.value = physicalAnimator.MaximumAngularVelocity;
            maxAngularVelocityField.isDelayed = true;
            maxAngularVelocityField.RegisterValueChangedCallback(evt => { physicalAnimator.MaximumAngularVelocity = evt.newValue; SetDollAndPartsDirty(); });
            baseElement.Add(maxAngularVelocityField);



            physicsBonesList = new DropList("Physics Bones");
            baseElement.Add(physicsBonesList);

            string[] boneLabels = new string[physicalAnimator.physicsBones.Length];
            if (physicalAnimator.animationSource)
            {
                for (int i = 0; i < boneLabels.Length; i++)
                {
                    boneLabels[i] = physicalAnimator.animationSource.animationBones[i].name;
                }
            }
            else
            {
                for (int i = 0; i < boneLabels.Length; i++)
                {
                    boneLabels[i] = physicalAnimator.physicsBones[i].transform.name;
                }
            }

            physicsBonesList.BuildList(boneLabels);
            physicsBonesList.OnSelect += SelectBone;

            selectedBoneField = new VisualElement();
            baseElement.Add(selectedBoneField);
            selectedBoneField.style.display = DisplayStyle.None;


            return baseElement;
        }
        private void SetDollAndPartsDirty()
        {
            EditorUtility.SetDirty(physicalAnimator.gameObject);
            if (physicalAnimator.animationSource)
            {
                for (int i = 0; i < physicalAnimator.physicsBones.Length; i++)
                {
                    EditorUtility.SetDirty(physicalAnimator.animationSource.animationBones[i]);
                    EditorUtility.SetDirty(physicalAnimator.physicsBones[i].transform);
                    EditorUtility.SetDirty(physicalAnimator.physicsBones[i].synchronizer);
                }
            }
            else
            {
                for (int i = 0; i < physicalAnimator.physicsBones.Length; i++)
                {
                    EditorUtility.SetDirty(physicalAnimator.physicsBones[i].transform);
                }
            }
        }

        private void SelectBone(int id)
        {
            selectedBoneField.Clear();

            if (id < 0 || id >= physicalAnimator.physicsBones.Length) { return; }

            selectedBoneField.style.display = DisplayStyle.Flex;
            int symmetryGroupID = -1;
            
            if (physicalAnimator.animationSource)
            {
                
                var groups = physicalAnimator.animationSource.groups;
                for (int i = 0; i < groups.Length; i++)
                {
                    if (groups[i].symmetryType != AnimationGroupSymmetry.Bilateral ||
                        groups[i].bones.Length != 2) { continue; }

                    if (groups[i].boneIDs[0] == id)
                    {
                        symmetryGroupID = groups[i].boneIDs[1];
                        break;
                    }
                    else if (groups[i].boneIDs[1] == id)
                    {
                        symmetryGroupID = groups[i].boneIDs[0];
                        break;
                    }
                }
            }

            //Labels for the transforms: For now, lets REQUIRE finding and syncing them through Setup Skeleton.
            //Enum drops for Collider Type, and Joint Type.
            if (symmetryGroupID >= 0)
            {
                Button applyMirror = new Button(() => { physicalAnimator.ApplyPartAToPartB(id, symmetryGroupID); })
                { text = "Apply to " + physicalAnimator.animationSource.animationBones[symmetryGroupID].name };
                selectedBoneField.Add(applyMirror);
            }
            Label animatedTransformLabel = new Label("Animated Transform:  " + physicalAnimator.animationSource.animationBones[id].name);
            selectedBoneField.Add(animatedTransformLabel);

            Label physicsTransformLabel = new Label("Physics Transform:  " + physicalAnimator.physicsBones[id].transform.name);
            selectedBoneField.Add(physicsTransformLabel);

            TextField colliderField = new TextField("Collider Type");
            colliderField.value = physicalAnimator.physicsBones[id].colliderType;
            colliderField.isDelayed = true;
            colliderField.RegisterValueChangedCallback(evt =>
            {
                physicalAnimator.ChangeBoneCollider(id, evt.newValue);
            });
            selectedBoneField.Add(colliderField);

            TextField jointField = new TextField("Joint Type");
            jointField.value = physicalAnimator.physicsBones[id].jointType;
            jointField.isDelayed = true;
            jointField.RegisterValueChangedCallback(evt =>
            {
                physicalAnimator.ChangeBoneJoint(id, evt.newValue);
            });
            selectedBoneField.Add(jointField);
        }
    }
}