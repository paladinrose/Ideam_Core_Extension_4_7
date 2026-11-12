#if UNITY_EDITOR

using System;
using System.IO;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.UIElements;
using UnityEngine.SceneManagement;

using UnityEditor;
using UnityEditor.UIElements;
using UnityEditor.SceneManagement;

using Cinemachine;

using Ideam;
using Ideam.Unity.UI;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Unity.Science.Natural.Cosmology;

namespace Ideam.Editor
{

    #region Editors and Property Drawers

    public abstract class UIElementsEditor : UnityEditor.Editor
    {
        protected virtual string[] ExcludedProperties()
        {
            return new string[] { "m_Script", "m_ObjectHideFlags" };
        }

        protected VisualElement root;
        protected Foldout defaultInspector;

        protected StyleSheet ideOS;

        protected virtual void OnEnable() { SceneView.duringSceneGui += OnSceneGUI; }

        protected virtual void OnDisable() { SceneView.duringSceneGui -= OnSceneGUI; }

        public override VisualElement CreateInspectorGUI()
        {
            ideOS = AssetDatabase.LoadAssetAtPath<StyleSheet>("Assets/Ideam Group/Unity Implementation/UI/Ideos.uss");
            
            root = CreateIdeamGUI();
            root.AddToClassList("ideamRoot");
            root.AddToClassList("background");
            root.styleSheets.Add(ideOS);
            
            return root;
        }

        public virtual VisualElement CreateIdeamGUI()
        {
            root = new VisualElement();
            root.Add(CreateDefaultInspectorGUI());
            return root;
        }

        protected Foldout CreateDefaultInspectorGUI()
        {
            //root.styleSheets.Add(Ideam.DefaultUI.GetStyle());
            defaultInspector = new Foldout() { name = "DefaultInspector", value = false, text = "Default Inspector"  };

            string[] excluded = ExcludedProperties();

            SerializedProperty iterator = serializedObject.GetIterator();
            iterator.Next(true);

            do
            {
                bool makeField = true;
                for (int i = 0; i < excluded.Length; i++)
                {
                    if (excluded[i] == iterator.name) { makeField = false; break; }
                }
                if (makeField)
                {
                    PropertyField property = new PropertyField(iterator);
                    property.Bind(serializedObject);
                    defaultInspector.Add(property);
                }
            }
            while (iterator.NextVisible(false));

            if(defaultInspector.childCount == 0) { defaultInspector.style.display = DisplayStyle.None; }

            return defaultInspector;
        }

        public override void OnInspectorGUI()
        {
            serializedObject.Update();
            
            base.OnInspectorGUI();
            if (GUI.changed)
            {
                EditorUtility.SetDirty(serializedObject.targetObject);
                PrefabUtility.RecordPrefabInstancePropertyModifications(serializedObject.targetObject);
            }
            serializedObject.ApplyModifiedProperties();
        }

        protected virtual void OnSceneGUI(SceneView sceneView)
        {

        }

        protected virtual void UpdateObject()
        {
            serializedObject.Update();
            
            EditorUtility.SetDirty(serializedObject.targetObject);
            
            PrefabUtility.RecordPrefabInstancePropertyModifications(serializedObject.targetObject);
            
            serializedObject.ApplyModifiedProperties();
        }
    }

    /// <summary>
    /// Formerly named FieldElementsEditor
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <typeparam name="TUI"></typeparam>
    public abstract class EditorUserInterface<T, TUI> : UIElementsEditor
        where T:UnityEngine.Object
        where TUI : UserInterface<T>
    {
        protected T editorObject;
        protected TUI ui;

        
        public override VisualElement CreateInspectorGUI()
        {
            SetupEditorObject();
            base.CreateInspectorGUI();
            root.Add(ui.OpenUserInterface());
            return root;
        }

        public override VisualElement CreateIdeamGUI()
        {
            return base.CreateIdeamGUI();
        }

        protected virtual void SetupEditorObject()
        {
            editorObject = (T)target;
            ui = CreateInstance<TUI>();
            ui.SetSourceObject(editorObject);
            ui.OnUIUpdate += UpdateObject;
        }

        private void OnDestroy()
        {
            if (ui)
            {
                ui.UnsubscribeFromObjectEvents();
                DestroyImmediate(ui);
            }
        }

    }

    #endregion

    #region Editor Extensions
    public static class EditorExtension
    {
        #region Tools and Utilities

        public class SceneSetupInfo
        {
            public Scene scene;

            public GameObject[] rootObjects;
            
            public Camera mainCamera;
            public CinemachineBrain mainCameraBrain;

            public Light mainDirectionalLight;

            public Volume globalVolume;
            public UnityCosmos cosmos;
            public UnityGameBoard gameBoard;
        }

        [MenuItem("Tools/Ideam/Scenes/Create Cosmos Scene")]
        public static void SetupCosmosScene()
        {
            Scene cosmosScene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects);
            EditorSceneManager.SetActiveScene(cosmosScene);
            

            GameObject cosmosObject = new GameObject("Cosmos");
            UnityCosmos cosmos = cosmosObject.AddComponent<UnityCosmos>();

            EditorSceneManager.SaveScene(cosmosScene);

            SceneSetupInfo setupScene = SetupScene();

            setupScene.cosmos = cosmos;

            EditorSceneManager.SaveScene(cosmosScene);
        }

        public static SceneSetupInfo SetupCosmosScene(string cosmosPath)
        {
            Scene cosmosScene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects);
            EditorSceneManager.SetActiveScene(cosmosScene);


            GameObject cosmosObject = new GameObject("Cosmos");
            UnityCosmos cosmos = cosmosObject.AddComponent<UnityCosmos>();

            EditorSceneManager.SaveScene(cosmosScene, cosmosPath);

            SceneSetupInfo setupScene = SetupScene();

            setupScene.cosmos = cosmos;

            EditorSceneManager.SaveScene(cosmosScene);
            return setupScene;
        }

        [MenuItem("Tools/Ideam/Scenes/Create Game Scene")]
        public static void SetupGameScene()
        {
            Scene gameScene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects);
            EditorSceneManager.SetActiveScene(gameScene);
            

            GameObject boardObject = new GameObject("Game Board");
            UnityGameBoard board = boardObject.AddComponent<UnityGameBoard>();
            EditorSceneManager.SaveScene(gameScene);

            SceneSetupInfo setupScene = SetupScene();
            setupScene.gameBoard = board;
            EditorSceneManager.SaveScene(gameScene);
        }

        public static SceneSetupInfo SetupGameScene(UnityGame game, string sceneName, string scenePath)
        {

            Scene gameScene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Additive);
            
            EditorSceneManager.SetActiveScene(gameScene);

            GameObject boardObject = new GameObject("Game Board");
            UnityGameBoard board = boardObject.AddComponent<UnityGameBoard>();
            board.game = game;

            EditorSceneManager.SaveScene(gameScene, scenePath);
            SceneSetupInfo setupScene = SetupScene();
            setupScene.gameBoard = board;
            EditorSceneManager.SaveScene(gameScene);

            return setupScene;
        }

        public static SceneSetupInfo SetupScene()
        {
            SceneSetupInfo setupInfo = new SceneSetupInfo();
            
            setupInfo.scene = EditorSceneManager.GetActiveScene();

            setupInfo.rootObjects = setupInfo.scene.GetRootGameObjects();

            for(int i =0; i < setupInfo.rootObjects.Length; i++)
            {
                GameObject rootObject = setupInfo.rootObjects[i];

                if (setupInfo.mainCamera == null)
                {
                    Camera[] cams = rootObject.GetComponentsInChildren<Camera>();
                    for (int c = 0; c < cams.Length; c++)
                    {
                        Camera currentCam = cams[c];

                        if (currentCam.tag == "MainCamera")
                        {
                            setupInfo.mainCamera = currentCam;
                            break;
                        }
                    }
                }

                if (setupInfo.mainDirectionalLight == null)
                {
                    Light[] lights = rootObject.GetComponentsInChildren<Light>();
                    for(int l =0; l < lights.Length; l++)
                    {
                        Light light = lights[l];
                        if(light.type == LightType.Directional)
                        {
                            setupInfo.mainDirectionalLight = light;
                            break;
                        }
                    }
                }

                if (setupInfo.globalVolume == null)
                {
                    Volume[] vols = rootObject.GetComponentsInChildren<Volume>();
                    for(int v = 0; v < vols.Length; v++)
                    {
                        if (vols[v].isGlobal) { setupInfo.globalVolume = vols[v]; break; }
                    }
                }
            }

            if (setupInfo.mainCamera)
            {
                setupInfo.mainCameraBrain = setupInfo.mainCamera.GetComponent<CinemachineBrain>();
                if(setupInfo.mainCameraBrain == null)
                {
                    setupInfo.mainCameraBrain = setupInfo.mainCamera.gameObject.AddComponent<CinemachineBrain>();
                }
            }

            if (setupInfo.mainDirectionalLight)
            {
                RenderSettings.sun = setupInfo.mainDirectionalLight;
            }

            if(setupInfo.globalVolume == null)
            {
                setupInfo.globalVolume = new GameObject("Global Volume").AddComponent<Volume>();
                setupInfo.globalVolume.isGlobal = true;
            }

            return setupInfo;
        }

        #endregion


        #region Gizmo Drawing

        public static void DrawWires(Vector3[] points)
        {
            for(int i =0; i < points.Length-1; i+=2)
            {
                Handles.DrawLine(points[i], points[i + 1]);
            }
        }

        public static void DrawWireBounds(Bounds bounds, Vector3 rotationPoint, Quaternion rotation)
        {
            Vector3[] boundsPoints = new Vector3[24];

            Vector3 urf = bounds.max, 
                    llb = bounds.min;
            Vector3 ulf = urf; ulf.x = llb.x;
            Vector3 llf = llb; llf.z = urf.z;
            Vector3 lrf = llf; lrf.x = urf.x;
            Vector3 urb = urf; urb.z = llb.z;
            Vector3 lrb = llb; lrb.x = urf.x;
            Vector3 ulb = llb; ulb.y = urf.y;

            Vector3 angles = rotation.eulerAngles;
            ulf = ulf.RotateAround(rotationPoint, angles);
            urf = urf.RotateAround(rotationPoint, angles);
            ulb = ulb.RotateAround(rotationPoint, angles);
            urb = urb.RotateAround(rotationPoint, angles);
            
            llf = llf.RotateAround(rotationPoint, angles);
            lrf = lrf.RotateAround(rotationPoint, angles);
            llb = llb.RotateAround(rotationPoint, angles);
            lrb = lrb.RotateAround(rotationPoint, angles);

            boundsPoints[0] = ulf; boundsPoints[1] = llf;
            boundsPoints[2] = ulf; boundsPoints[3] = urf;
            boundsPoints[4] = ulf; boundsPoints[5] = ulb;

            boundsPoints[6] = llf; boundsPoints[7] = lrf;
            boundsPoints[8] = llf; boundsPoints[9] = llb;
            boundsPoints[10] = lrf; boundsPoints[11] = urf;

            boundsPoints[12] = urf; boundsPoints[13] = urb;
            boundsPoints[14] = ulb; boundsPoints[15] = urb;
            boundsPoints[16] = ulb; boundsPoints[17] = llb;

            boundsPoints[18] = llb; boundsPoints[19] = lrb;
            boundsPoints[20] = lrf; boundsPoints[21] = lrb;
            boundsPoints[22] = urb; boundsPoints[23] = lrb;

            DrawWires(boundsPoints);
        }

        #endregion
    }
    #endregion
}

#endif