# Game Description

## 1. Opis Ogólny i Cel Projektu
Głównym celem projektu jest wytworzenie dwuwymiarowej (2D) wieloosobowej gry sieciowej z widokiem z góry (Top-Down Shooter). Projekt skupia się na implementacji wydajnej architektury sieciowej client-server, ze szczególnym uwzględnieniem mechanizmów synchronizacji stanu gry, płynności rozgrywki oraz ograniczenia opóźnień sieciowych (lagów) poprzez predykcję po stronie klienta oraz rekonsyliację serwera.

## 2. Przepływ Użytkownika i Struktura Lobby (UI Workflow)
Architektura interfejsu oraz przepływu stadiów gry opiera się na następujących ekranach i stanach:

1. Uruchomienie i Połączenie (Enter Game + Connect to Server):
   - Nawiązanie bezpiecznego połączenia z serwerem głównym.
2. Menu Główne (Main Menu):
   - Wyświetlanie ikony / profilu gracza.
   - Tworzenie lub dołączanie do istniejących pokoi/lobby.
   - Lista dostępnych gier.
3. Ekran Rozpoczynania Gry (Starting Game Screen):
   - Dołączenie do kolejki serwera.
   - Automatyczny przydział do drużyn przez serwer (Przydział losowy z uwzględnieniem “party” graczy).
4. Wybór Postaci (Champion Select Screen):
   - Wybór bohatera z dostępnej listy.
   - Podgląd członków drużyny w czasie rzeczywistym.
   - Odliczanie czasu do startu meczu.
5. Ekran Rozgrywki (Game Screen):
   - Główna pętla mechaniki gry (poruszanie, strzelanie, modyfikatory).
   - Detekcja warunków zakończenia gry.
6. Ekran Podsumowania (Post Game Screen):
   - Wyświetlanie listy graczy oraz statystyk meczu.
   - Przycisk opuszczenia gry / powrotu do menu.
