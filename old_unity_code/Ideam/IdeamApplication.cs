using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace Ideam
{
    
    /// <summary>
    /// Provides top level data management for files, directories, and preferences.
    /// </summary>
    public static class IdeamApplication
    {

        public static int[] IntegerSequence(int i)
        {
            int[] sequence = new int[i];
            for(int ii =0; ii < i; ii++)
            {
                sequence[ii] = ii;
            }
            return sequence;
        }
        public static bool InitializeApplication()
        {

            return true;
        }

        #region File Management

        public static bool SaveBytes(string path, byte[] bytes)
        {
            File.WriteAllBytes(path, bytes);
            return true;
        }

        public static bool SaveString(string path, string data)
        {
            File.WriteAllText(path, data);
            return true;
        }

        public static byte[] LoadBytes(string path)
        {
            return File.ReadAllBytes(path);
        }

        public static string LoadString(string path)
        {
            return File.ReadAllText(path);
        }

        #endregion
    }

    public interface IDataObject
    {
        int GetDataID<T>(T dataManager) where T : IDataManager;
        T GetDataManager<T>() where T : IDataManager;
        string SaveDataToString();
        void SaveData<T>() where T:IDataManager;
        void LoadDataFromString(string data);
        void LoadData<T>() where T:IDataManager;
    }
    public interface IDataManager
    {
        T GetDataObject<T>(int dataID) where T:IDataObject;
        T[] GetAllManagedData<T>() where T : IDataObject;
        int GetLocalDataID<T>(T dataManaged) where T : IDataObject;
    }
}
