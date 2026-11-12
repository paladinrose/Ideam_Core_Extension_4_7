using Ideam.Science.Formal.Mathematics.Information.Cryptography;

namespace Ideam.Science.Formal.Systems
{
    /// <summary>
    /// Users make use of the functions of a system to reach some sort of end. 
    /// Unlike other elements of a system, users are able to make choices that can define and alter the structure and functions of a system.
    /// </summary>
    public interface IUser 
    {
        T SecurityPass<T>() where T : ISecurityPass;

    }
}