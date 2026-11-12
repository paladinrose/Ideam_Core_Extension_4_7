using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using Ideam.Editor;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using UnityEngine.UIElements;

using Ideam.Unity.Science.Formal.Mathematics;
namespace Ideam.Unity.Science.Natural.Physics
{
    [CustomEditor(typeof(PhysicsRope))]
    public class PhysicsRopeEditor : UIElementsEditor
    {
        PhysicsRope ropeTarget;
        Spline3HandleDrawer ropeDrawer;

        protected override void OnEnable()
        {
            base.OnEnable();
            ropeTarget = (PhysicsRope)target;
            ropeDrawer = new Spline3HandleDrawer(new Spline3[] { ropeTarget.ropeSpline }, true);
            ropeDrawer.OnStopPointMove += UpdateRope;
        }
        
        protected override void OnDisable()
        {
            base.OnDisable();
            ropeDrawer.OnStopPointMove -= UpdateRope;
        }

        public override VisualElement CreateIdeamGUI()
        {
            VisualElement baseElement = base.CreateIdeamGUI();
            if (!ropeTarget.RopeIsGenerated)
            {
                baseElement.Add(new Button(() => { ropeTarget.GenerateRope(); }) { text = "Generate Rope" });
            }
            else
            {
                baseElement.Add(new Button(() => { ropeTarget.ResetAndGenerateRope(); }) { text = "Reset and Regenerate" });
            }
            return baseElement;
        }

        protected override void OnSceneGUI(SceneView sceneView)
        {
            ropeDrawer.DrawSplineAndHandles();
        }


        void UpdateRope()
        {
            ropeTarget.PositionRopeAlongSpline();
            EditorUtility.SetDirty(ropeTarget);
            PrefabUtility.RecordPrefabInstancePropertyModifications(ropeTarget);
            serializedObject.ApplyModifiedProperties();
        }
    }
}