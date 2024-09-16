# Bookmarking 

This introduces bookmarking functionality to the assistant queries. It allows users to bookmark specific queries from the session history and later reference or remove these bookmarks using a defined alias.

### Features

- **Bookmark last query** - Save the most recent query with a specified alias. Alias is unique and, if already taken, cannot be used multiple times.
  ```sh
  bookmark <alias>
  ```
- **Bookmark specific query** - Save a query from a specific history index with an alias. The index is counted from the most recent to older queries (1 being the most recent).
  ```sh
  bookmark <index> <alias>
  ```
- **Execute bookmarked query** - Run the query associated with the given alias.
  ```shell
  <alias> -b
  <alias> --bookmark
  ```
- **Remove bookmark** - Delete a bookmark by its alias.
  ```shell
  <alias> -r
  <alias> --remove
  ```
- **List all bookmarks** - Display all saved bookmarks.
  ```shell
  bookmark -l
  bookmark --list
  ```
- **Help command** - Display usage instructions and available commands related to bookmarks. The manual is available [here](https://github.com/smart-linux-shell/ishell/blob/32_bookmarking/tui-tux/manuals/bookmark.txt).
  ```shell
  bookmark -h
  bookmark --help
  ```
  
### Implementation Details

Queries are recorded during the session and are lost after it ends. Bookmarks are saved in a local `bookmarks.csv` file and persist across sessions.
The full code and implementation details are available [here](https://github.com/smart-linux-shell/ishell/blob/32_bookmarking/tui-tux/bookmarks.cpp).

### Future work

Potential future improvements include a graphical preview of bookmarks or interactive listing, and an extended manual.