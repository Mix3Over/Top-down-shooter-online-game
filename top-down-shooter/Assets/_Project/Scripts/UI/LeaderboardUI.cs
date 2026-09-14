using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;

public class LeaderboardUI : UIWindow
{
    [SerializeField] private Transform _leaderboardListParent;
    [SerializeField] private Color _normalColor;
    [SerializeField] private Color _localPlayerColor;

    private List<Transform> _children;

    private void Start()
    {
        _children = new List<Transform>(_leaderboardListParent.childCount);
        foreach (Transform child in _leaderboardListParent)
        {
            _children.Add(child);
            child.gameObject.SetActive(false);
        }

        UpdateLeaderboard();
    }

    private void OnEnable()
    {
        GameManager.PlayerScoresUpdated += UpdateLeaderboard;
    }

    private void OnDisable()
    {
        GameManager.PlayerScoresUpdated -= UpdateLeaderboard;
    }

    private void UpdateLeaderboard()
    {
        if (GameManager.Instance == null)
            return;

        List<Player> players = GameManager.Instance.Players.Values.ToList();

        players.Sort((p1, p2) => p2.Score.CompareTo(p1.Score));

        foreach (Transform child in _children)
            child.gameObject.SetActive(false);

        for (int i = 0; i < _children.Count &&  i < players.Count; i++)
        {
            _children[i].GetChild(0).GetComponent<TMP_Text>().text = (i + 1) + " " + players[i].PlayerName;
            _children[i].GetChild(0).GetComponent<TMP_Text>().color = GameManager.Instance.LocalPlayer == players[i] ? _localPlayerColor : _normalColor;
            _children[i].GetChild(1).GetComponent<TMP_Text>().text = players[i].Score.ToString();
            _children[i].gameObject.SetActive(true);
        }
    }
}
