using System;
using UnityEngine;

using UnityEngine.Rendering;

[ExecuteAlways]
public class CameraShadow : RelativeObjectShadow
{
    public Camera targetCamera, shadowCamera;
    public bool shadowFOV;
    float lastFOV;
    private void OnEnable()
    {
        if (!shadowCamera)
        {
            shadowCamera = gameObject.GetComponent<Camera>();
        }
        RenderPipelineManager.beginFrameRendering += ShadowCamera;
    }

    private void OnDisable()
    {
        RenderPipelineManager.beginFrameRendering -= ShadowCamera;
    }

    private void ShadowCamera(ScriptableRenderContext arg1, Camera[] arg2)
    {
        ShadowObject();
    }

    public override void ShadowObject()
    {
        base.ShadowObject();
        if (shadowFOV) { ShadowFOV(); }
    }

    protected virtual void ShadowFOV()
    {
        if(lastFOV == targetCamera.fieldOfView) { return; }

        lastFOV = targetCamera.fieldOfView;
        shadowCamera.fieldOfView = lastFOV;
    }
}
