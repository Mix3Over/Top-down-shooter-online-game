using System;
using System.Collections.Generic;
using System.Linq;
using Unity.Cinemachine;
using UnityEngine;

public class GameManager : Singleton<GameManager>
{
    public event Action PlayerScoresUpdated;

    [SerializeField] private List<PlayerTypeData> _playerTypes;
    [SerializeField] private CinemachineCamera _playerCamera;
    [SerializeField] private GameObject _playerPrefab;
    [SerializeField] private GameObject _bulletPrefab;
    [SerializeField] private Transform _playersParent;

    public IReadOnlyDictionary<int, Player> Players => _players;
    private readonly Dictionary<int, Player> _players = new ();
    private readonly Dictionary<int, Bullet> _bullets = new ();
    public Player LocalPlayer { get; private set; }

    private void Start()
    {
        Application.targetFrameRate = 60;
        UIManager.Instance.LobbyUI.Open();
    }

    private void OnEnable()
    {
        NetworkManager.Instance.Disconnected += OnDisconnected;
        NetworkManager.Instance.TcpMessageReceived += OnTcpMessageReceived;
        NetworkManager.Instance.UdpMessageReceived += OnUdpMessageReceived;
    }

    private void OnDisable()
    {
        NetworkManager.Instance.Disconnected -= OnDisconnected;
        NetworkManager.Instance.TcpMessageReceived -= OnTcpMessageReceived;
        NetworkManager.Instance.UdpMessageReceived -= OnUdpMessageReceived;
    }

    private void OnDisconnected()
    {
        foreach (Player player in _players.Values.Where(player => player != null))
            Destroy(player.gameObject);

        foreach (Bullet bullet in _bullets.Values.Where(bullet => bullet != null))
            Destroy(bullet.gameObject);
        
        _players.Clear();
        _bullets.Clear();
        LocalPlayer = null;

        if(UIManager.Instance != null && UIManager.Instance.LeaderboardUI) 
            UIManager.Instance.LeaderboardUI.Close();
    }

    public void SpawnLocalPlayer(int playerTypeIndex)
    {
        UIManager.Instance.LeaderboardUI.Open();
        PlayerTypeData playerTypeData = _playerTypes[playerTypeIndex];
        NetworkManager.Instance.TcpSendMessageToServer($"I;" +
            $"{NetworkManager.Instance.UdpLocalPort};" +
            $"{UIManager.Instance.LobbyUI.PlayerName};" +
            $"{playerTypeIndex};{playerTypeData.BulletDamage};" +
            $"{playerTypeData.BulletSpeed};" +
            $"{playerTypeData.StartingHealth};");
    }

    private void OnTcpMessageReceived(string message)
    {
        string[] parts = message.Split(";");

        if (parts.Length < 1) {
            Debug.LogWarning("Incorrect TCP message format!");
            return;
        }

        string trimmedMessage = message[1..].Trim(';');
        switch (parts[0]) {
            case "I":
                InitLocalPlayer(trimmedMessage);
                break;
            case "M":
                InitPlayers(trimmedMessage);
                break;
            case "C":
                AddRemotePlayer(trimmedMessage);
                break;
            case "D":
                RemovePlayer(trimmedMessage);
                break;
            case "S":
                SpawnBullet(trimmedMessage);
                break;
            case "H":
                PlayerHit(trimmedMessage);
                break;
            case "E":
                RemoveBullet(trimmedMessage);
                break;
            case "P":
                UpdatePlayerScores(trimmedMessage);
                break;
        }
    }

    private void UpdatePlayerScores(string message)
    {
        string[] playersData = message.Split("!");

        foreach (string playerData in playersData) {
            if(playerData.Trim() == string.Empty)
                continue;

            string[] parts = playerData.Split(";");

            if(parts.Length < 2) {
                Debug.LogWarning("[GM] Incorrect player score update format!: " + string.Join(',', parts));
                continue;
            }
            if (!int.TryParse(parts[0], out int playerId)) {
                Debug.LogWarning("[GM] Failed to parse player ID: " + parts[0]);
                continue;
            }
            if (!int.TryParse(parts[1], out int score)) {
                Debug.LogWarning("[GM] Failed to parse player score: " + parts[1]);
                continue;
            }

            if (!_players.TryGetValue(playerId, out Player player)) {
                Debug.LogWarning($"Player with ID:{playerId} doesn't exist");
                continue;
            }

            player.SetScore(score);
        }
        PlayerScoresUpdated?.Invoke();
    }

    private void RemoveBullet(string message)
    {
        string[] parts = message.Split(";");
        if (parts.Length > 0 && int.TryParse(parts[0], out int bulletId) && _bullets.TryGetValue(bulletId, out Bullet bullet)) {
            Destroy(bullet.gameObject);
            _bullets.Remove(bulletId);
        }
    }

    private void PlayerHit(string message)
    {
        string[] parts = message.Split(";");
        if (parts.Length < 3) {
            Debug.LogWarning("Incorrect player hit message format! " + string.Join(',', parts));
            return;
        }
        if (!int.TryParse(parts[0], out int playerId)) {
            Debug.LogWarning("[GM] Failed to parse player ID: " + parts[0]);
            return;
        }
        if (!int.TryParse(parts[1], out int bulletId)) {
            Debug.LogWarning("[GM] Failed to parse bullet ID: " + parts[1]);
            return;
        }
        if (!int.TryParse(parts[2], out int health)) {
            Debug.LogWarning("[GM] Failed to parse health: " + parts[2]);
            return;
        }
        if (!int.TryParse(parts[3], out int isAliveInt)) {
            Debug.LogWarning("[GM] Failed to parse isAlive: " + parts[3]);
            return;
        }

        if(!_bullets.TryGetValue(bulletId, out Bullet bullet)) {
            Debug.LogWarning($"Bullet with ID:{bulletId} doesn't exist");
            return;
        }

        Destroy(bullet.gameObject);

        if (!_players.TryGetValue(playerId, out Player player)) {
            Debug.LogWarning($"Player with ID:{playerId} doesn't exist");
            return;
        }

        player.SetHealth(health);

        if (isAliveInt != 0) return;
        
        PlayerDeath(playerId);
    }

    private void PlayerDeath(int playerId)
    {
        if(!_players.TryGetValue(playerId, out Player player)) {
            Debug.LogWarning($"Player with ID:{playerId} doesn't exist");
            return;
        }

        Destroy(player.gameObject);

        if(playerId == NetworkManager.Instance.PlayerID) {
            UIManager.Instance.InfoUI.Open("You died", "Play again", UIManager.Instance.PlayerTypeSelectionUI.Open);
        } else {
            _players.Remove(playerId);
        }
    }

    private void SpawnBullet(string message)
    {
        string[] parts = message.Split(";");
        if (parts.Length < 5) {
            Debug.LogWarning("Incorrect bullet spawn message format! " + string.Join(',', parts));
            return;
        }
        if (!int.TryParse(parts[0], out int bulletId)) {
            Debug.LogWarning("[GM] Failed to parse bullet ID: " + parts[0]);
            return;
        }
        if (!int.TryParse(parts[1], out int shooterId)) {
            Debug.LogWarning("[GM] Failed to parse shooter ID: " + parts[1]);
            return;
        }

        if (!int.TryParse(parts[2], out int positionX)) {
            Debug.LogWarning("[GM] Failed to parse position X: " + parts[2]);
            return;
        }
        if (!int.TryParse(parts[3], out int positionY)) {
            Debug.LogWarning("[GM] Failed to parse position Y: " + parts[3]);
            return;
        }
        if (!int.TryParse(parts[4], out int rotation)) {
            Debug.LogWarning("[GM] Failed to parse rotation: " + parts[4]);
            return;
        }

        GameObject bulletObject = Instantiate(_bulletPrefab);
        Bullet bullet = bulletObject.GetComponent<Bullet>();
        _bullets[bulletId] = bullet;

        Vector2Int position = new(positionX, positionY);

        if(!_players.TryGetValue(shooterId, out Player player)) {
            Debug.LogWarning($"Player with ID:{shooterId} doesn't exist");
            return;
        }

        float speed = player.PlayerTypeData.BulletSpeed;
        bullet.Init(position.ToLocal(), rotation, speed);
    }

    private void RemovePlayer(string message)
    {
        Debug.Log("Remove player from message: " + message);
        if (!int.TryParse(message, out int playerId)) {
            Debug.LogWarning("[GM] Failed to parse player ID: " + message);
            return;
        }
        if (!_players.TryGetValue(playerId, out Player player)) {
            Debug.LogWarning($"Player with ID:{playerId} doesn't exist");
            return;
        }

        Destroy(player.gameObject);
        _players.Remove(playerId);
    }

    private void OnUdpMessageReceived(string message)
    {
        string[] playersData = message.TrimStart('M', ';').Split("!");

        foreach (string playerData in playersData) {
            if(playerData == string.Empty)
                continue;

            string[] parts = playerData.Split(";");
            if (parts.Length < 4) {
                Debug.LogWarning("[GM] Incorrect udp message format!: " + string.Join(',', parts));
                continue;
            }
            if (!int.TryParse(parts[0], out int playerId)) {
                Debug.LogWarning("[GM] Failed to parse player ID: " + parts[0]);
                continue;
            }
            if (!int.TryParse(parts[1], out int positionX)) {
                Debug.LogWarning("[GM] Failed to parse positionX");
                continue;
            }
            if (!int.TryParse(parts[2], out int positionY)) {
                Debug.LogWarning("[GM] Failed to parse position Y");
                continue;
            }
            if (!int.TryParse(parts[3], out int rotation)) {
                Debug.LogWarning("[GM] Failed to parse rotation");
                continue;
            }

            if (!_players.TryGetValue(playerId, out Player player)) {
                Debug.LogWarning($"Player with ID:{playerId} doesn't exist");
                continue;
            }

            if (LocalPlayer == player)
                continue;

            Vector2Int position = new(positionX, positionY);
            player.SetPositionAndRotation(position.ToLocal(), rotation);
        }
    }

    private void InitLocalPlayer(string message)
    {
        Debug.Log("Init local player from message: " + message);
        string[] parts = message.Split(";");

        if(parts.Length < 2) {
            Debug.LogWarning("[GM] Incorrect local player init message format!");
            return;
        }

        Vector2Int position = new(int.Parse(parts[0]), int.Parse(parts[1]));
        SpawnLocalPlayer(position.ToLocal(), UIManager.Instance.PlayerTypeSelectionUI.SelectedPlayerType);
    }

    private void SpawnLocalPlayer(Vector3 position, int playerTypeIndex)
    {
        GameObject playerObject = Instantiate(_playerPrefab, _playersParent);
        playerObject.AddComponent<PlayerController>();

        LocalPlayer = playerObject.GetComponent<Player>();
        LocalPlayer.Init(_playerTypes[playerTypeIndex], UIManager.Instance.LobbyUI.PlayerName, -1);

        LocalPlayer.SetPositionAndRotation(position, 0f);
        _players[NetworkManager.Instance.PlayerID] = LocalPlayer;
        _playerCamera.Target.TrackingTarget = LocalPlayer.transform;
    }

    private void SpawnRemotePlayer(Vector3 position, int playerTypeIndex, int playerId, string playerName, int health)
    {
        GameObject playerObject = Instantiate(_playerPrefab, _playersParent);

        Player player = playerObject.GetComponent<Player>();
        player.Init(_playerTypes[playerTypeIndex], playerName, health);
        player.SetPositionAndRotation(position, 0f);
        _players[playerId] = player;
    }

    private void AddRemotePlayer(string message)
    {
        if (message == string.Empty)
            return;
        string[] parts = message.Split(";");
        if (!int.TryParse(parts[0], out int playerId)) {
            Debug.LogWarning("[GM] Failed to parse player ID: " + parts[0]);
            return;
        }
        string playerName = parts[1];

        if (!int.TryParse(parts[2], out int playerTypeIndex)) {
            Debug.LogWarning("[GM] Failed to parse playerType: " + parts[2]);
            return;
        }

        if (!int.TryParse(parts[3], out int positionX)) {
            Debug.LogWarning("[GM] Failed to parse position X: " + parts[3]);
            return;
        }

        if (!int.TryParse(parts[4], out int positionY)) {
            Debug.LogWarning("[GM] Failed to parse position Y");
            return;
        }
        if (!int.TryParse(parts[5], out int _)) {
            Debug.LogWarning("[GM] Failed to parse rotation");
            return;
        }
        int health = -1;
        if(parts.Length > 6) {
            if (!int.TryParse(parts[6], out health)) {
                Debug.LogWarning("[GM] Failed to parse player health");
                return;
            }
        }

        if (NetworkManager.Instance.PlayerID == playerId)
            return;

        if (_players.ContainsKey(playerId)) {
            Debug.LogWarning($"Player with ID:{playerId} already exist");
            return;
        }

        Vector2Int position = new(positionX, positionY);
        SpawnRemotePlayer(position.ToLocal(), playerTypeIndex, playerId, playerName, health);
    }

    private void InitPlayers(string message)
    {
        Debug.Log("Init players from message: " + message);
        string[] playersData = message.Split("!");

        foreach (string playerData in playersData) {
            AddRemotePlayer(playerData);
        }
    }
}
