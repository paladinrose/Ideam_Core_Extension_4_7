using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RelativeObjectShadow : MonoBehaviour
{
    public enum OffsetSpace
    {
        World, SelfScaled, SelfUnscaled, TargetScaled, TargetUnscaled
    }

    public Transform targetObject;

    public bool shadowPosition, shadowRotation, shadowScale;
    

    public OffsetSpace positionOffsetSpace = OffsetSpace.TargetScaled, 
                       rotationOffsetSpace = OffsetSpace.TargetScaled, 
                       scaleOffsetSpace = OffsetSpace.TargetScaled;

    public Vector3 positionOffset = Vector3.zero, positionScaling = Vector3.one,
                   rotationOffset = Vector3.zero, rotationScaling = Vector3.one,
                   scaleOffset = Vector3.zero, scaleScaling = Vector3.one;
    
    Vector3 lastPosition, lastRotation, lastScale;
     
    
    public virtual void ShadowObject()
    {
        if (!targetObject) return;
        if (shadowScale) { ShadowObjectScale(); }
        if (shadowRotation) { ShadowObjectRotation(); }
        if (shadowPosition) { ShadowObjectPosition(); }
    }

    protected virtual void ShadowObjectScale() 
    {
        if (targetObject.localScale == lastScale) return;
        lastScale = targetObject.localScale;
        switch (scaleOffsetSpace)
        {
            case OffsetSpace.World:
                //Lossy Scale
                break;

            case OffsetSpace.SelfScaled:
                break;

            case OffsetSpace.SelfUnscaled:
                break;

            case OffsetSpace.TargetScaled:
                
                Vector3 scale = lastScale;
                scale.Scale(scaleScaling);
                scale += scaleOffset;

                transform.localScale = scale;

                break;

            case OffsetSpace.TargetUnscaled:
                break;
        }
        
    }

    protected virtual void ShadowObjectRotation() 
    {
        if (targetObject.eulerAngles == lastRotation) return;
        
        switch (rotationOffsetSpace)
        {
            case OffsetSpace.World:
                
                lastRotation = targetObject.eulerAngles;
        
                Vector3 rotation = lastRotation;
                rotation.Scale(rotationScaling);

                Quaternion rot = Quaternion.Euler(rotation);
        
                rot *= Quaternion.Euler(rotationOffset);

                transform.rotation = rot;
        
                //transform.eulerAngles = rotation;

                break;

            case OffsetSpace.SelfScaled:
                break;
            case OffsetSpace.SelfUnscaled:
                break;
            case OffsetSpace.TargetScaled:
                break;
            case OffsetSpace.TargetUnscaled:
                break;
        }

        

    }

    protected virtual void ShadowObjectPosition() 
    {
        if (targetObject.position == lastPosition) return;
        Vector3 position;
        switch (positionOffsetSpace)
        {
            case OffsetSpace.World:
                lastPosition = targetObject.position;
                position = lastPosition;
                position.Scale(positionScaling);
                position += positionOffset;

                transform.position = position;
                break;

            case OffsetSpace.SelfScaled:
                break;
            case OffsetSpace.SelfUnscaled:
                break;
            case OffsetSpace.TargetScaled:
                break;

            case OffsetSpace.TargetUnscaled:
                lastPosition = targetObject.position;
                position = lastPosition;
                position.Scale(positionScaling);
                position += targetObject.TransformDirection(positionOffset);

                transform.position = position;
                break;
        }
        

    }
}
