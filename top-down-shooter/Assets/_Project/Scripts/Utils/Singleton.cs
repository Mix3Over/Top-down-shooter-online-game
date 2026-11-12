using UnityEngine;

public abstract class Singleton<T> : MonoBehaviour where T : MonoBehaviour
{
    public static T Instance { get; private set; }

    protected virtual void Awake()
    {
        if (Instance != null)
        {
            Destroy(gameObject);
            Debug.LogError($"[Singleton] Found multiple: '{typeof(T)}'");
            return;
        }
        Instance = this as T;
    }

    protected virtual void OnDestroy()
    {
        if (Instance != this)
        {
            Debug.LogWarning($"[Singleton] Instance of '{typeof(T)}' was destroyed or overwritten.");
            return;
        }
        Instance = null;
    }
}