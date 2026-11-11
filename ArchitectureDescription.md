# Architecture Description

## 1. Architektura Sieciowa i Wzorzec Dedykowany
- Architektura sieciowa: Client-server.
- Architektura serwera: Dedykowany serwer (C++). Serwer pozostaje ostatecznym autorytetem stanu gry.
- Przeznaczenie architektury: Zapewnienie wydajnej synchronizacji stanu gry, płynności rozgrywki oraz ograniczenia opóźnień sieciowych (lagów).

## 2. Protokół Sieciowy i Bezpieczeństwo
- Protokół: QUIC (Quick UDP Internet Connections).
- Cel zastosowania: Zapewnienie szyfrowania, integralności danych oraz niskich opóźnień (Low-Latency Transport).

## 3. Synchronizacja Stanu i Obsługa Opóźnień
- Client-Side Prediction (Predykcja po stronie klienta): Klient lokalnie symuluje ruch własnej postaci natychmiast po zarejestrowaniu wejścia (Input System), nie czekając na odpowiedź z serwera, co wyklucza odczucie opóźnienia.
- Server Reconciliation (Rekonsyliacja serwera): Serwer przesyła potwierdzone pozycje, a klient koryguje ewentualne rozbieżności pomiędzy stanem przewidzianym a autorytatywnym.

## 4. Organizacja Kodu i Zadań Programistycznych
- Modułowość Serwera: Podział głównego pliku `serwer.cpp` na mniejsze, wyspecjalizowane moduły i pliki źródłowe.
- Separacja Scen: Rozdzielenie logiki sceny rozgrywki od interfejsu użytkownika.
- Standaryzacja Kodu: Wdrożenie spójnych konwencji nazewnictwa oraz refaktoryzacja struktury katalogów i prefabów UI.
- Infrastruktura: Dobór odpowiedniego dostawcy hostingu dla serwera gier oraz bazy danych.
