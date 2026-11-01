using UnityEngine;

[CreateAssetMenu(fileName = "DebugConnectionConfig", menuName = "Debug Connection Config")]
public class DebugConnectionConfig : ScriptableObject
{
    [field: SerializeField] public string IpAddress { get; private set; }
    [field: SerializeField] public int Port { get; private set; }
}