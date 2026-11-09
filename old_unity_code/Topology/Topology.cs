using System.Collections.Generic;

using UnityEngine;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;
using System;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public static class Topology
    {
        public static ISymmetry GetSymmetryTemplate(SymmetryGroupType symmetryType)
        {
            switch (symmetryType)
            {
                case SymmetryGroupType.P1:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);
                    
                case SymmetryGroupType.P2:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.PM:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.PG:
                    return new Symmetry6(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.CM:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P2MM:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P2MG:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P2GG:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.C2MM:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P4:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P4MM:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P4MG:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P3:
                    return new Symmetry4(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P3M1:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P31M:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P6:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);

                case SymmetryGroupType.P6MM:
                    return new Symmetry3(Float2.zero, Float2.one, 0f);
            }

            return default;
        }

        public static Matrix3x3[] GetSymmetryTransformations(SymmetryGroupType tilingType)
        {
            switch (tilingType)
            {
                case SymmetryGroupType.P1:
                    return new Matrix3x3[]
                    {
                        Matrix3x3.TranslationMatrix(new Float2(1, 0)),
                        Matrix3x3.TranslationMatrix(new Float2(0, -1)),
                        Matrix3x3.TranslationMatrix(new Float2(-1, 0)),
                        Matrix3x3.TranslationMatrix(new Float2(0, 1))
                    };
                    
                case SymmetryGroupType.P2:
                    return new Matrix3x3[]
                    {
                        Matrix3x3.TranslationMatrix(new Float2(1, 0)),
                        Matrix3x3.TranslationMatrix(new Float2(0, -1)),
                        Matrix3x3.RotationAroundPointMatrix(new Float2(0, -0.5f), 180f),
                        Matrix3x3.RotationAroundPointMatrix(new Float2(0, 0.5f), 180f),
                    };
                    
                case SymmetryGroupType.PM:
                    return new Matrix3x3[]
                    {
                        Matrix3x3.TranslationMatrix(new Float2(0, 1)),
                        Matrix3x3.TranslationMatrix(new Float2(0, -1)),
                        Matrix3x3.ReflectionMatrix(new Float2(-0.5f, -0.5f), new Float2(0.5f, 0.5f)),
                        Matrix3x3.ReflectionMatrix(new Float2(-0.5f, -0.5f), new Float2(0.5f, 0.5f)),

                    };
                    
                case SymmetryGroupType.PG:
                    return new Matrix3x3[]
                    {
                        Matrix3x3.TranslationMatrix(new Float2(0.5f, 1)),
                        Matrix3x3.TranslationMatrix(new Float2(-0.5f, 1)),
                        Matrix3x3.TranslationMatrix(new Float2(0.5f, -1)),
                        Matrix3x3.TranslationMatrix(new Float2(-0.5f, -1)),
                        Matrix3x3.TranslationMatrix(new Float2(-1, 0)),
                        Matrix3x3.TranslationMatrix(new Float2(0, 1))
                    };
                    break;
                case SymmetryGroupType.CM:
                    break;
                case SymmetryGroupType.P2MM:
                    break;
                case SymmetryGroupType.P2MG:
                    break;
                case SymmetryGroupType.P2GG:
                    break;
                case SymmetryGroupType.C2MM:
                    break;
                case SymmetryGroupType.P4:
                    break;
                case SymmetryGroupType.P4MM:
                    break;
                case SymmetryGroupType.P4MG:
                    break;
                case SymmetryGroupType.P3:
                    break;
                case SymmetryGroupType.P3M1:
                    break;
                case SymmetryGroupType.P31M:
                    break;
                case SymmetryGroupType.P6:
                    break;
                case SymmetryGroupType.P6MM:
                    break;
            }

            return new Matrix3x3[0];
        }

        public static Float2[] GetSymmetryVertices(SymmetryGroupType symmetryType)
        {

            switch (symmetryType)
            {
                case SymmetryGroupType.P1:
                    return Quad();

                case SymmetryGroupType.P2:
                    return Quad();

                case SymmetryGroupType.PM:
                    return Quad();

                case SymmetryGroupType.PG:
                    return Brick();

                case SymmetryGroupType.CM:
                    return FlatTriangle();

                case SymmetryGroupType.P2MM:
                    return Quad();

                case SymmetryGroupType.P2MG:
                    return Quad();

                case SymmetryGroupType.P2GG:
                    return Quad();

                case SymmetryGroupType.C2MM:
                    return RightTriangle();

                case SymmetryGroupType.P4:
                    return Quad();

                case SymmetryGroupType.P4MM:
                    return RightTriangle();

                case SymmetryGroupType.P4MG:
                    return RightTriangle();

                case SymmetryGroupType.P3:
                    return Rhombus();

                case SymmetryGroupType.P3M1:
                    return EquilateralTriangle();

                case SymmetryGroupType.P31M:
                    return FlatTriangle();

                case SymmetryGroupType.P6:
                    return FlatTriangle();

                case SymmetryGroupType.P6MM:
                    return TallTriangle();
            }

            return new Float2[0];
        }

        #region Four Sided

        public static Float2[] Quad(float scale = 1)
        {
            return new Float2[]
            {
                new Float2(-0.5f, -0.5f)*scale,
                new Float2(-0.5f, 0.5f)*scale,
                new Float2(0.5f, 0.5f)*scale,
                new Float2(0.5f, -0.5f)*scale
            };
        }

        public static Float2[] Rhombus(float scale = 1)
        {
            return new Float2[]
            {
                new Float2 (-0.5f, -0.28865f)*scale,
                new Float2(-0.16667f, 0.28865f)*scale,
                new Float2(0.5f, 0.28865f)*scale,
                new Float2(0.16667f, -0.28865f)*scale
            };
        }

        #endregion

        #region Three Sided

        public static Float2[] RightTriangle(float scale = 1)
        {
            return new Float2[]
            {
                new Float2(-0.5f, -0.5f)*scale,
                new Float2(0.5f, 0.5f)*scale,
                new Float2(0.5f, -0.5f)*scale
            };
        }

        public static Float2[] EquilateralTriangle(float scale = 1)
        {
            return new Float2[]
            {
                new Float2 (-0.5f, -0.28875f)*scale,
                new Float2(0, 0.5775f)*scale,
                new Float2(0.5f, -0.28875f)*scale
            };
        }

        public static Float2[] TallTriangle(float scale = 1)
        {
            return new Float2[]
            {
                new Float2 (-0.28868f, -0.5f)*scale,
                new Float2(0.28868f, 0.5f)*scale,
                new Float2(0.28868f, -0.5f)*scale
            };
        }

        public static Float2[] FlatTriangle(float scale = 1)
        {
            return new Float2[]
            {
                new Float2 (-0.5f, -0.14434f)*scale,
                new Float2(0, 0.14434f)*scale,
                new Float2(0.5f, -0.14434f)*scale
            };
        }

        #endregion

        #region Six Sided

        public static Float2[] Brick(float scale = 1)
        {
            return new Float2[]
            {
                new Float2 (-0.5f, -0.5f)*scale,
                new Float2 (-0.5f, 0.5f)*scale,
                new Float2 (0, 0.5f)*scale,
                new Float2 (0.5f, 0.5f)*scale,
                new Float2 (0.5f, -0.5f)*scale,
                new Float2 (0, -0.5f)*scale
            };
        }

        #endregion

        public static void EquilateralHexSplats(int splatSize, out Float3[] pix, out int[] splatCountIDs)
        {
            List<Float3> pixels = new List<Float3>();
            List<int> splatCounts = new List<int>();
            int maskCount = 0;
            Float2[] cornerPoints = EquilateralTriangle(splatSize-1),
                     bottomVerts = new Float2[7],
                     leftSideVerts = new Float2[7],
                     rightSideVerts = new Float2[7];

            
            Float2 splatOffset = Float2.zero;
            for(int i = 0; i < cornerPoints.Length; i++)
            {
                if(cornerPoints[i].x < splatOffset.x) { splatOffset.x = cornerPoints[i].x; }
                if (cornerPoints[i].y < splatOffset.y) { splatOffset.y = cornerPoints[i].y; }
            }
            splatOffset = +splatOffset;

            splatOffset += 1;

            cornerPoints[0] += splatOffset;
            cornerPoints[1] += splatOffset;
            cornerPoints[2] += splatOffset;

            bottomVerts[0] = cornerPoints[0];
            bottomVerts[1] = Float2.Lerp(cornerPoints[0], cornerPoints[2], 0.2f);
            bottomVerts[2] = Float2.Lerp(cornerPoints[0], cornerPoints[2], 0.333334f);
            bottomVerts[3] = Float2.Lerp(cornerPoints[0], cornerPoints[2], 0.5f);
            bottomVerts[4] = Float2.Lerp(cornerPoints[0], cornerPoints[2], 0.666667f);
            bottomVerts[5] = Float2.Lerp(cornerPoints[0], cornerPoints[2], 0.8f);
            bottomVerts[6] = cornerPoints[2];

            leftSideVerts[0] = cornerPoints[0];
            leftSideVerts[1] = Float2.Lerp(cornerPoints[0], cornerPoints[1], 0.2f);
            leftSideVerts[2] = Float2.Lerp(cornerPoints[0], cornerPoints[1], 0.333334f);
            leftSideVerts[3] = Float2.Lerp(cornerPoints[0], cornerPoints[1], 0.5f);
            leftSideVerts[4] = Float2.Lerp(cornerPoints[0], cornerPoints[1], 0.666667f);
            leftSideVerts[5] = Float2.Lerp(cornerPoints[0], cornerPoints[1], 0.8f);
            leftSideVerts[6] = cornerPoints[1];

            rightSideVerts[0] = cornerPoints[1];
            rightSideVerts[1] = Float2.Lerp(cornerPoints[1], cornerPoints[2], 0.2f);
            rightSideVerts[2] = Float2.Lerp(cornerPoints[1], cornerPoints[2], 0.333334f);
            rightSideVerts[3] = Float2.Lerp(cornerPoints[1], cornerPoints[2], 0.5f);
            rightSideVerts[4] = Float2.Lerp(cornerPoints[1], cornerPoints[2], 0.666667f);
            rightSideVerts[5] = Float2.Lerp(cornerPoints[1], cornerPoints[2], 0.8f);
            rightSideVerts[6] = cornerPoints[2];

            
            Float3[] maskPix = new Float3[splatSize * splatSize];
            float[] masked;

            //Black
            pixels.AddRange(maskPix);
            maskCount++;

            //Solid Triangle
            masked = GetTrianglePixels(splatSize, cornerPoints);
            for (int i = 0; i < masked.Length; i++)
            {
                maskPix[i] = new Float3(masked[i], 0, 0); 
            }
            pixels.AddRange(maskPix);
            maskCount++;
            splatCounts.Add(maskCount);

            //Two Splats
            for (int b = 1; b < 6; b++)
            {
                for(int l = 1; l < 7; l++)
                {
                    
                    Float2[] subTri = new Float2[] 
                    {
                        bottomVerts[0],
                        leftSideVerts[l],
                        bottomVerts[b]
                    };
                    masked = GetTrianglePixels(splatSize, subTri);
                    for(int i = 0; i < masked.Length; i++)
                    {
                        maskPix[i] = new Float3(masked[i], 0, 0); 
                    }
                    pixels.AddRange(maskPix);
                    maskCount++;

                    if (l == 6)
                    {
                        for (int r = 1; r < 6; r++)
                        {
                            subTri = new Float2[]
                            {
                                bottomVerts[b],
                                leftSideVerts[l],
                                rightSideVerts[r]
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for(int i = 0; i < masked.Length; i++)
                            {
                                if(maskPix[i] == Float3.zero) { maskPix[i] = new Float3(masked[i],0,0); }
                            }
                            pixels.AddRange(maskPix);
                            maskCount++;
                        }
                    }
                }
            }
            splatCounts.Add(maskCount);

            //Three Splats
            for (int b = 1; b < 7; b++)
            {
                for(int l = 0; l < 6; l++)
                {
                    for(int r = 0; r < 6; r++)
                    {
                        Float2 midPoint = bottomVerts[b] + leftSideVerts[l] + rightSideVerts[r];
                        midPoint /= 3f;

                        Float2[] subTri = new Float2[]
                        {
                            bottomVerts[0],
                            midPoint,
                            bottomVerts[b]
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for(int i = 0; i < masked.Length; i++)
                        {
                            maskPix[i] = new Float3(masked[i], 0, 0);
                        }

                        subTri = new Float2[]
                        {
                            bottomVerts[0],
                            leftSideVerts[l],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if(maskPix[i] == Float3.zero)maskPix[i] = new Float3(masked[i],0, 0);
                        }

                        subTri = new Float2[]
                        {
                            leftSideVerts[l],
                            leftSideVerts[6],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(masked[i], masked[i], 0);
                        }

                        subTri = new Float2[]
                        {
                            leftSideVerts[6],
                            rightSideVerts[r],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(masked[i],masked[i], 0);
                        }

                        subTri = new Float2[]
                        {
                            rightSideVerts[r],
                            rightSideVerts[6],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(0,masked[i],0);
                        }

                        subTri = new Float2[]
                        {
                            rightSideVerts[6],
                            bottomVerts[b],
                            midPoint
                        };

                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(0,masked[i],0);
                        }

                        pixels.AddRange(maskPix);
                        maskCount++;
                    }
                }
            }
            splatCounts.Add(maskCount);

            //Four Splats
            for (int b = 1; b < 7; b++)
            {
                for (int l = 0; l < 6; l++)
                {
                    for (int r = 0; r < 6; r++)
                    {
                        
                        if (b == 6 && l == 0 && r == 0) { continue; }

                        for (int c = 0; c < 3; c++)
                        {
                            Float2 midPoint = bottomVerts[b] + leftSideVerts[l] + 
                                              rightSideVerts[r] + cornerPoints[c];
                            midPoint /= 4f;

                            Float2[] subTri = new Float2[]
                            {
                            bottomVerts[0],
                            midPoint,
                            bottomVerts[b]
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                maskPix[i] = new Float3(masked[i], 0, 0);
                            }

                            subTri = new Float2[]
                            {
                                bottomVerts[0],
                                leftSideVerts[l],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero) 
                                {
                                    switch (c)
                                    {
                                        case 0:
                                            maskPix[i] = new Float3(masked[i], masked[i], 0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(masked[i], 0, 0);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                leftSideVerts[l],
                                leftSideVerts[6],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 0:
                                            maskPix[i] = new Float3(0, masked[i], 0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(masked[i], masked[i], 0);
                                            break;
                                       
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                leftSideVerts[6],
                                rightSideVerts[r],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 2:
                                            maskPix[i] = new Float3(masked[i], masked[i],0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(0, masked[i], 0);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                rightSideVerts[r],
                                rightSideVerts[6],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 2:
                                            maskPix[i] = new Float3(0, masked[i], 0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(0, masked[i], masked[i]);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                rightSideVerts[6],
                                bottomVerts[b],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    maskPix[i] = new Float3(0, masked[i], masked[i]);
                                }
                            }
                            pixels.AddRange(maskPix);
                            maskCount++;
                        }
                    }
                }
            }
            splatCounts.Add(maskCount);

            //Five Splats
            for (int b = 1; b < 7; b++)
            {
                for (int l = 0; l < 6; l++)
                {
                    for (int r = 0; r < 6; r++)
                    {

                        if (b == 6 && l == 0 && r == 0) { continue; }

                        for (int c = 0; c < 3; c++)
                        {
                            Float2 midPoint = bottomVerts[b] + leftSideVerts[l] +
                                              rightSideVerts[r];
                            switch (c)
                            {
                                case 0:
                                    midPoint += cornerPoints[1] + cornerPoints[2];
                                    break;
                                case 1:
                                    midPoint += cornerPoints[0] + cornerPoints[2];
                                    break;
                                case 2:
                                    midPoint += cornerPoints[0] + cornerPoints[1];
                                    break;
                            }
                            midPoint /= 5f;

                            Float2[] subTri = new Float2[]
                            {
                            bottomVerts[0],
                            midPoint,
                            bottomVerts[b]
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                maskPix[i] = new Float3(masked[i], 0, 0);
                            }

                            subTri = new Float2[]
                            {
                                bottomVerts[0],
                                leftSideVerts[l],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 0:
                                            maskPix[i] = new Float3(masked[i], 0,0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(masked[i], masked[i], 0);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                leftSideVerts[l],
                                leftSideVerts[6],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 0:
                                            maskPix[i] = new Float3(masked[i], masked[i], 0);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(0, masked[i], 0);
                                            break;

                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                leftSideVerts[6],
                                rightSideVerts[r],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 2:
                                            maskPix[i] = new Float3(0, masked[i], masked[i]);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(0, masked[i], 0);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                rightSideVerts[r],
                                rightSideVerts[6],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    switch (c)
                                    {
                                        case 2:
                                            maskPix[i] = new Float3(0, 0, masked[i]);
                                            break;
                                        default:
                                            maskPix[i] = new Float3(0, masked[i], masked[i]);
                                            break;
                                    }
                                }
                            }

                            subTri = new Float2[]
                            {
                                rightSideVerts[6],
                                bottomVerts[b],
                                midPoint
                            };
                            masked = GetTrianglePixels(splatSize, subTri);
                            for (int i = 0; i < masked.Length; i++)
                            {
                                if (maskPix[i] == Float3.zero)
                                {
                                    maskPix[i] = new Float3(0, 0, masked[i]);
                                }
                            }
                            pixels.AddRange(maskPix);
                            maskCount++;
                        }
                    }
                }
            }
            splatCounts.Add(maskCount);


            //Six Splats
            for (int b = 1; b < 7; b++)
            {
                for (int l = 0; l < 6; l++)
                {
                    for (int r = 0; r < 6; r++)
                    {
                        if (b ==6 && l == 0 && r == 0) { continue; }

                        Float2 midPoint = bottomVerts[b] + leftSideVerts[l] + rightSideVerts[r];
                        midPoint /= 3f;

                        Float2[] subTri = new Float2[]
                        {
                            bottomVerts[0],
                            midPoint,
                            bottomVerts[b]
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            maskPix[i] = new Float3(masked[i], 0, 0);
                        }

                        subTri = new Float2[]
                        {
                            bottomVerts[0],
                            leftSideVerts[l],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if(maskPix[i] == Float3.zero) maskPix[i] = new Float3(masked[i], masked[i], 0);
                        }

                        subTri = new Float2[]
                        {
                            leftSideVerts[l],
                            leftSideVerts[6],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(0, masked[i], 0);
                        }

                        subTri = new Float2[]
                        {
                            leftSideVerts[6],
                            rightSideVerts[r],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(0, masked[i], masked[i]);
                        }

                        subTri = new Float2[]
                        {
                            rightSideVerts[r],
                            rightSideVerts[6],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(0, 0, masked[i]);
                        }

                        subTri = new Float2[]
                        {
                            rightSideVerts[6],
                            bottomVerts[b],
                            midPoint
                        };
                        masked = GetTrianglePixels(splatSize, subTri);
                        for (int i = 0; i < masked.Length; i++)
                        {
                            if (maskPix[i] == Float3.zero) maskPix[i] = new Float3(masked[i], 0, masked[i]);
                        }

                        pixels.AddRange(maskPix);
                        maskCount++;
                    }
                }
            }
            splatCounts.Add(maskCount);

            pix = pixels.ToArray();
            splatCountIDs = splatCounts.ToArray();
        }

        public static float[] GetTrianglePixels(int size, Float2[] tri)
        {
            float[] triPix = new float[size * size];
            for(int x= 0; x < size; x++)
            {
                for(int y = 0; y < size; y++)
                {
                    if(Float2.InsideTriangle(tri[0], tri[1], tri[2], new Float2(x, y)))
                    {
                        triPix[y * size + x] = 1f;
                    }
                    else
                    {
                        triPix[y * size + x] = 0;
                    }
                }
            }
            return triPix;
        }


        
    }

    public enum LatticeType
    {
        Square, Rectangular, Rhombic, Hexagonal, Parallelogrammatic
    }

    public interface ITiling
    {
        Float2[] AxesOfSymmetry { get; }
        Float2[] PointsOfRotation { get; }
        Float2[] Glides { get; }
        Float2[] TileVertices { get; }
        int[] TileTriangles { get; }

        LatticeType LatticeType { get; }
        Matrix3x3[] Symmetries { get; }

        void RebuildSymmetries();
    }

    public interface ITile
    {
        Float2 Position { get; }
        Float2 Scale { get; }
        Float Rotation { get; }
    }

    [Serializable]
    public struct BaseTile : ITile
    {
        public Float2 Position { get; set; }
        public Float2 Scale { get; set; }
        public Float Rotation { get; set; }
    }

    public abstract class BaseTiling : ITiling
    {
        public Float2[] axes = new Float2[0];
        public virtual Float2[] AxesOfSymmetry 
        {
            get { return axes; } 
            set 
            { 
                if(axes != value) 
                { 
                    axes = value; 
                    RebuildSymmetries(); 
                }
            }
        }

        protected Float2[] tileVertices = new Float2[0];
           public Float2[] pointsOfRotation = new Float2[0],
                           glides = new Float2[0];
        
        protected int[] tileTriangles = new int[0];
        
        public abstract Float2[] TileVertices { get; }
        public abstract int[] TileTriangles { get; }
        public abstract LatticeType LatticeType { get; }
        public abstract Matrix3x3[] Symmetries { get; }
        public virtual Float2[] PointsOfRotation { get { return pointsOfRotation; } set { if (pointsOfRotation != value) { pointsOfRotation = value; RebuildSymmetries(); } } } 
        public virtual Float2[] Glides { get { return glides; } set { if(glides != value) { glides = value; RebuildSymmetries(); } } }
        
        
        public abstract void RebuildSymmetries();
    }

    public abstract class SquareSymmetryGroup : BaseTiling
    {
        public override Float2[] TileVertices { get { if (tileVertices.Length == 0) tileVertices = Topology.Quad(); return tileVertices; } }

        public override int[] TileTriangles { get { if (tileTriangles.Length == 0) tileTriangles = new int[] { 0, 1, 3, 1, 2, 3 }; return tileTriangles; } }
        public override LatticeType LatticeType { get { return LatticeType.Square; } }

    }
    public abstract class RectangularSymmetryGroup : BaseTiling
    {
        public override Float2[] TileVertices { get { if (tileVertices.Length == 0) tileVertices = Topology.Quad(); return tileVertices; } }

        public override int[] TileTriangles { get { if (tileTriangles.Length == 0) tileTriangles = new int[] { 0, 1, 3, 1, 2, 3 }; return tileTriangles; } }
        public override LatticeType LatticeType { get { return LatticeType.Rectangular; } }

    }


    public abstract class RhombicSymmetryGroup : BaseTiling
    {
        public override Float2[] TileVertices { get { if (tileVertices.Length == 0) tileVertices = Topology.Rhombus(); return tileVertices; } }

        public override int[] TileTriangles { get { if (tileTriangles.Length == 0) tileTriangles = new int[] { 0, 1, 3, 1, 2, 3 }; return tileTriangles; } }
        public override LatticeType LatticeType { get { return LatticeType.Rhombic; } }

    }

    public abstract class ParallelogrammaticSymmetryGroup : BaseTiling
    {
        public override Float2[] TileVertices { get { if (tileVertices.Length == 0) tileVertices = Topology.Quad(); return tileVertices; } }

        public override int[] TileTriangles { get { if (tileTriangles.Length == 0) tileTriangles = new int[] { 0, 1, 3, 1, 2, 3 }; return tileTriangles; } }
        public override LatticeType LatticeType { get { return LatticeType.Parallelogrammatic; } }
    }

    [Serializable]
    public class P1SymmetryGroup : ParallelogrammaticSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { new Float2(0,1), new Float2(1,0) }; }
                return axes;
            }
            set 
            {
                if(value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 translateMajorPositive, translateMajorNegative, translateMinorPositive, translateMinorNegative;
        public override Matrix3x3[] Symmetries 
        { 
            get 
            {
                if(translateMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[] 
                {translateMajorPositive, translateMajorNegative, translateMinorPositive, translateMinorNegative }; 
            }
        }

        public override Float2[] PointsOfRotation { get => pointsOfRotation; set { } }
        public override Float2[] Glides { get => glides; set { } }

        public override void RebuildSymmetries() 
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];
            translateMajorPositive = Matrix3x3.TranslationMatrix(majorAxis);
            translateMajorNegative = Matrix3x3.TranslationMatrix(-majorAxis);
            translateMinorPositive = Matrix3x3.TranslationMatrix(minorAxis);
            translateMinorNegative = Matrix3x3.TranslationMatrix(-minorAxis);
        }
    }

    public class P2SymmetryGroup : ParallelogrammaticSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) }; }
                return axes;
            }
            set
            {
                if (value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 translateMajorPositive, translateMajorNegative, rotateMinorPositive, rotateMinorNegative;

        public override Matrix3x3[] Symmetries
        {
            get
            {
                if(translateMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {translateMajorPositive, translateMajorNegative, rotateMinorPositive, rotateMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation
        {
            get { if (pointsOfRotation.Length == 0) { Float2 minorAxis = axes[1]; pointsOfRotation = new Float2[] { minorAxis, -minorAxis}; } return pointsOfRotation; }
            set { if (value.Length == 2 && pointsOfRotation != value) { pointsOfRotation = value; RebuildSymmetries(); } }
        }

        public override Float2[] Glides { get => glides; set { } }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            translateMajorPositive = Matrix3x3.TranslationMatrix(majorAxis);
            translateMajorNegative = Matrix3x3.TranslationMatrix(-majorAxis);
            rotateMinorPositive = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180f);
            rotateMinorNegative = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 180f);
        }
    }

    public class PMSymmetryGroup : RectangularSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) }; }
                return axes;
            }
            set
            {
                if (value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 translateMajorPositive, translateMajorNegative, reflectMinorPositive, reflectMinorNegative;
        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (translateMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {translateMajorPositive, translateMajorNegative, reflectMinorPositive, reflectMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation { get => pointsOfRotation; set { } }

        public override Float2[] Glides { get => glides; set { } }
        
        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];
            translateMajorPositive = Matrix3x3.TranslationMatrix(majorAxis);
            translateMajorNegative = Matrix3x3.TranslationMatrix(-majorAxis);
            reflectMinorPositive = Matrix3x3.ReflectionMatrix(majorAxis + minorAxis, -majorAxis + minorAxis);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(majorAxis - minorAxis, -majorAxis - minorAxis);
        }
    }

    public class PGSymmetryGroup : RectangularSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { }; }
                return axes;
            }
            set
            {
                if (value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 translateMajorPositive, translateMajorNegative, glideReflectMinorPositive, glideReflectMinorNegative;
        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (translateMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {translateMajorPositive, translateMajorNegative, glideReflectMinorPositive, glideReflectMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation 
        { 
            get => pointsOfRotation; 
            set 
            { 
                if (value.Length == 1 && pointsOfRotation != value) { pointsOfRotation = value; RebuildSymmetries(); } 
            }
        }

        public override Float2[] Glides 
        {
            get => glides;
        
            set 
            {
                if (value.Length == 1 && value != glides) 
                {
                    glides = value; 
                    RebuildSymmetries();
                } 
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];
            translateMajorPositive = Matrix3x3.TranslationMatrix(majorAxis);
            translateMajorNegative = Matrix3x3.TranslationMatrix(-majorAxis);
            glideReflectMinorPositive = Matrix3x3.GlideReflectionMatrix(majorAxis + minorAxis, -majorAxis + minorAxis, glides[0]);
            glideReflectMinorNegative = Matrix3x3.GlideReflectionMatrix(majorAxis - minorAxis, -majorAxis - minorAxis, -glides[0]);
        }
    }

    public class CMSymmetryGroup : RhombicSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { }; }
                return axes;
            }
            set
            {
                if (value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 translateMajorPositive, translateMajorNegative, glideReflectMinorPositive, reflectMinorNegative;
        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (translateMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {translateMajorPositive, translateMajorNegative, glideReflectMinorPositive, reflectMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation
        {
            get => pointsOfRotation;
            set
            {
                if (value.Length == 1 && pointsOfRotation != value) { pointsOfRotation = value; RebuildSymmetries(); }
            }
        }

        public override Float2[] Glides
        {
            get {
                if (glides.Length == 0) { glides = new Float2[] { }; }
                return glides;
            }

            set
            {
                if (value.Length == 1 && value != glides)
                {
                    glides = value;
                    RebuildSymmetries();
                }
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];

            translateMajorPositive = Matrix3x3.TranslationMatrix(majorAxis);
            translateMajorNegative = Matrix3x3.TranslationMatrix(-majorAxis);
            glideReflectMinorPositive = Matrix3x3.GlideReflectionMatrix(majorAxis + minorAxis, -majorAxis + minorAxis, glides[0]);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(majorAxis - minorAxis, -majorAxis - minorAxis);
        }
    }

    public class PMMSymmetryGroup : RectangularSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { }; }
                return axes;
            }
            set
            {
                if (value.Length == 2 && axes != value) { axes = value; RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative;
        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (reflectMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation { get => pointsOfRotation; set { } }

        public override Float2[] Glides { get => glides; set { } }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];

            reflectMajorPositive = Matrix3x3.ReflectionMatrix(minorAxis + majorAxis, -minorAxis + majorAxis);
            reflectMajorNegative = Matrix3x3.ReflectionMatrix(minorAxis - majorAxis, -minorAxis - majorAxis);
            reflectMinorPositive = Matrix3x3.ReflectionMatrix(majorAxis + minorAxis, -majorAxis + minorAxis);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(majorAxis - minorAxis, -majorAxis - minorAxis);
        }
    }

    public class PMGSymmetryGroup : RectangularSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { }; }
                return axes;
            }
            set
            {
                if (value.Length == 1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 reflectMajorPositive, reflectMajorNegative, rotateMinorPositive, rotateMinorNegative;
        
        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (reflectMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {reflectMajorPositive, reflectMajorNegative, rotateMinorPositive, rotateMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation 
        {
            get 
            { 
                if (pointsOfRotation.Length == 0) 
                {
                    Float2 majorAxis = axes[0];
                    pointsOfRotation = new Float2[] 
                    {
                        majorAxis, -majorAxis
                    }; 
                } 
                return pointsOfRotation; 
            }
            set 
            {
            } 
        }

        public override Float2[] Glides { get => glides; set { } }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];
            reflectMajorPositive = Matrix3x3.ReflectionMatrix(minorAxis + majorAxis, -minorAxis + majorAxis);
            reflectMajorNegative = Matrix3x3.ReflectionMatrix(minorAxis - majorAxis, -minorAxis - majorAxis);
            rotateMinorPositive = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180);
            rotateMinorNegative = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 180);
        }
    }

    public class PGGSymmetryGroup : RectangularSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { }; }
                return axes;
            }
            set
            {
                if (value.Length == 1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 glideReflectMajorPositive, glideReflectMajorNegative, rotateMinorPositive, rotateMinorNegative;

        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (glideReflectMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {glideReflectMajorPositive, glideReflectMajorNegative, rotateMinorPositive, rotateMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation
        {
            get
            {
                if (pointsOfRotation.Length == 0)
                {
                    Float2 majorAxis = axes[0];
                    pointsOfRotation = new Float2[]
                    {
                        majorAxis*0.5f, majorAxis*-0.5f
                    };
                }
                return pointsOfRotation;
            }
            set
            {
                if(value.Length ==2 && value != pointsOfRotation) { pointsOfRotation = value; RebuildSymmetries(); }
            }
        }

        public override Float2[] Glides
        {
            get
            {
                if (glides.Length == 0)
                {
                    Float2 minorAxis = axes[1];
                    glides = new Float2[]
                    {
                        minorAxis, -minorAxis
                    };
                }
                return glides;
            }
            set
            {
                if (value.Length == 2 && value != glides) { glides = value; RebuildSymmetries(); }
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];
            glideReflectMajorPositive = Matrix3x3.GlideReflectionMatrix(minorAxis + majorAxis, -minorAxis + majorAxis, glides[0]);
            glideReflectMajorNegative = Matrix3x3.GlideReflectionMatrix(minorAxis - majorAxis, -minorAxis - majorAxis, glides[1]);
            rotateMinorPositive = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180);
            rotateMinorNegative = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 180);
        }
    }

    public class CMMSymmetryGroup : RhombicSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0) { axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) }; }
                return axes;
            }
            set
            {
                if (value.Length == 1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); RebuildSymmetries(); }
            }
        }

        protected Matrix3x3 reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative,
            glideReflectMajorPositive, glideReflectMajorNegative, glideReflectMinorPositive, glideReflectMinorNegative,
            rotateAtReflectionAxes, rotateAtGlideReflectionAxes;

        public override Matrix3x3[] Symmetries
        {
            get
            {
                if (reflectMajorPositive == default) { RebuildSymmetries(); }
                return new Matrix3x3[]
                {reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative };
            }
        }

        public override Float2[] PointsOfRotation
        {
            get
            {
                if (pointsOfRotation.Length == 0)
                {
                    Float2 majorAxis = axes[0];

                    pointsOfRotation = new Float2[]
                    {
                        majorAxis*0.5f, majorAxis*-0.5f
                    };
                }
                return pointsOfRotation;
            }
            set
            {
                if (value.Length == 2 && value != pointsOfRotation) { pointsOfRotation = value; RebuildSymmetries(); }
            }
        }

        public override Float2[] Glides
        {
            get
            {
                if (glides.Length == 0)
                {
                    Float2 minorAxis = axes[1];
                    glides = new Float2[]
                    {
                        minorAxis, -minorAxis
                    };
                }
                return glides;
            }
            set
            {
                if (value.Length == 2 && value != glides) { glides = value; RebuildSymmetries(); }
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];

            Float2 majorHalf = majorAxis * 0.5f;
            Float2 minorHalf = minorAxis * 0.5f;

            reflectMajorPositive = Matrix3x3.ReflectionMatrix(minorAxis + majorAxis, minorAxis-majorAxis);
            reflectMajorNegative = Matrix3x3.ReflectionMatrix(-minorAxis + majorAxis, -minorAxis - majorAxis);

            reflectMinorPositive = Matrix3x3.ReflectionMatrix(majorAxis + minorAxis, majorAxis - minorAxis);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(-majorAxis + minorAxis, -majorAxis - minorAxis);

            glideReflectMajorPositive = Matrix3x3.GlideReflectionMatrix(minorHalf + majorHalf, minorHalf - majorHalf, glides[0]);
            glideReflectMajorNegative = Matrix3x3.GlideReflectionMatrix(-minorHalf + majorHalf, -minorHalf - majorHalf, glides[0]);

            glideReflectMinorPositive = Matrix3x3.GlideReflectionMatrix(majorHalf + minorHalf, majorHalf - minorHalf, glides[1]);
            glideReflectMinorNegative = Matrix3x3.GlideReflectionMatrix(-majorHalf + minorHalf, -majorHalf - minorHalf, glides[1]);

            rotateAtReflectionAxes = Matrix3x3.RotationAroundPointMatrix(majorAxis + minorAxis, 180f);
            rotateAtGlideReflectionAxes = Matrix3x3.RotationAroundPointMatrix(majorHalf + minorHalf, 180f);
        }
    }

    public class P4SymmetryGroup : SquareSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry 
        {
            get 
            {
                if (axes.Length == 0) 
                {
                    axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) };
                }
                return axes; 
            } 
            set 
            {
                if(value.Length ==1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); }
            } 
        }

        protected Matrix3x3 rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4;

        public override Matrix3x3[] Symmetries { get { return new Matrix3x3[] { rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4 }; } }

        public override Float2[] PointsOfRotation 
        {
            get 
            {
                if(pointsOfRotation.Length == 0)
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];
                    pointsOfRotation = new Float2[5];
                    pointsOfRotation[0] = Float2.zero;
                    pointsOfRotation[1] = majorAxis - minorAxis;
                    pointsOfRotation[2] = majorAxis + minorAxis;
                    pointsOfRotation[3] = -majorAxis + minorAxis;
                    pointsOfRotation[4] = -majorAxis - minorAxis;
                }
                return pointsOfRotation;
            } 
            set 
            { 
                if(value.Length == 1 && pointsOfRotation[0] != value[0])
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];
                    
                    pointsOfRotation = new Float2[5];

                    pointsOfRotation[0] = value[0];
                    pointsOfRotation[1] = value[0] + majorAxis - minorAxis;
                    pointsOfRotation[2] = value[0] + majorAxis + minorAxis;
                    pointsOfRotation[3] = value[0] - majorAxis + minorAxis;
                    pointsOfRotation[4] = value[0] - majorAxis - minorAxis;

                }
            } 
        }

        public override void RebuildSymmetries()
        {
            rotateHalfTurn = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180f);
            rotateQuarter1 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 90f);
            rotateQuarter2 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[2], 90f);
            rotateQuarter3 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[3], 90f);
            rotateQuarter4 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[4], 90f);
        }

    }

    public class P4MMSymmetryGroup : SquareSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0)
                {
                    axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) };
                }
                return axes;
            }
            set
            {
                if (value.Length == 1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); }
            }
        }

        protected Matrix3x3 rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4, 
                            reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative;

        public override Matrix3x3[] Symmetries { get { return new Matrix3x3[] { rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4 }; } }

        public override Float2[] PointsOfRotation
        {
            get
            {
                if (pointsOfRotation.Length == 0)
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];
                    pointsOfRotation = new Float2[5];
                    pointsOfRotation[0] = Float2.zero;
                    pointsOfRotation[1] = majorAxis - minorAxis;
                    pointsOfRotation[2] = majorAxis + minorAxis;
                    pointsOfRotation[3] = -majorAxis + minorAxis;
                    pointsOfRotation[4] = -majorAxis - minorAxis;
                }
                return pointsOfRotation;
            }
            set
            {
                if (value.Length == 1 && pointsOfRotation[0] != value[0])
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];

                    pointsOfRotation = new Float2[5];

                    pointsOfRotation[0] = value[0];
                    pointsOfRotation[1] = value[0] + majorAxis - minorAxis;
                    pointsOfRotation[2] = value[0] + majorAxis + minorAxis;
                    pointsOfRotation[3] = value[0] - majorAxis + minorAxis;
                    pointsOfRotation[4] = value[0] - majorAxis - minorAxis;

                }
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];

            rotateHalfTurn = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180f);
            rotateQuarter1 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 90f);
            rotateQuarter2 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[2], 90f);
            rotateQuarter3 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[3], 90f);
            rotateQuarter4 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[4], 90f);

            reflectMajorPositive = Matrix3x3.ReflectionMatrix(minorAxis + majorAxis, minorAxis - majorAxis);
            reflectMajorNegative = Matrix3x3.ReflectionMatrix(-minorAxis + majorAxis, -minorAxis - majorAxis);

            reflectMinorPositive = Matrix3x3.ReflectionMatrix(majorAxis + minorAxis, majorAxis - minorAxis);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(-majorAxis + minorAxis, -majorAxis - minorAxis);
        }
    }

    public class P4MGSymmetryGroup : SquareSymmetryGroup
    {
        public override Float2[] AxesOfSymmetry
        {
            get
            {
                if (axes.Length == 0)
                {
                    axes = new Float2[] { new Float2(0, 1), new Float2(1, 0) };
                }
                return axes;
            }
            set
            {
                if (value.Length == 1 && axes[0] != value[0]) { axes[0] = value[0]; axes[1] = axes[0].Perpendicular(); }
            }
        }

        protected Matrix3x3 rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4,
                            reflectMajorPositive, reflectMajorNegative, reflectMinorPositive, reflectMinorNegative;

        public override Matrix3x3[] Symmetries { get { return new Matrix3x3[] { rotateHalfTurn, rotateQuarter1, rotateQuarter2, rotateQuarter3, rotateQuarter4 }; } }

        public override Float2[] PointsOfRotation
        {
            get
            {
                if (pointsOfRotation.Length == 0)
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];
                    pointsOfRotation = new Float2[5];
                    pointsOfRotation[0] = Float2.zero;
                    pointsOfRotation[1] = majorAxis - minorAxis;
                    pointsOfRotation[2] = majorAxis + minorAxis;
                    pointsOfRotation[3] = -majorAxis + minorAxis;
                    pointsOfRotation[4] = -majorAxis - minorAxis;
                }
                return pointsOfRotation;
            }
            set
            {
                if (value.Length == 1 && pointsOfRotation[0] != value[0])
                {
                    Float2 majorAxis = axes[0];
                    Float2 minorAxis = axes[1];

                    pointsOfRotation = new Float2[5];

                    pointsOfRotation[0] = value[0];
                    pointsOfRotation[1] = value[0] + majorAxis - minorAxis;
                    pointsOfRotation[2] = value[0] + majorAxis + minorAxis;
                    pointsOfRotation[3] = value[0] - majorAxis + minorAxis;
                    pointsOfRotation[4] = value[0] - majorAxis - minorAxis;

                }
            }
        }

        public override void RebuildSymmetries()
        {
            Float2 majorAxis = axes[0];
            Float2 minorAxis = axes[1];

            rotateHalfTurn = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[0], 180f);
            rotateQuarter1 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[1], 90f);
            rotateQuarter2 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[2], 90f);
            rotateQuarter3 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[3], 90f);
            rotateQuarter4 = Matrix3x3.RotationAroundPointMatrix(pointsOfRotation[4], 90f);

            reflectMajorPositive = Matrix3x3.ReflectionMatrix(minorAxis + majorAxis, minorAxis - majorAxis);
            reflectMajorNegative = Matrix3x3.ReflectionMatrix(-minorAxis + majorAxis, -minorAxis - majorAxis);

            reflectMinorPositive = Matrix3x3.ReflectionMatrix(majorAxis + minorAxis, majorAxis - minorAxis);
            reflectMinorNegative = Matrix3x3.ReflectionMatrix(-majorAxis + minorAxis, -majorAxis - minorAxis);
        }
    }
}
