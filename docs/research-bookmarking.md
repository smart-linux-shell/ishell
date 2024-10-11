# Research Report: Bookmark Listing Functionality

The bookmark system is designed to help users save time by enabling quick access to recurring commands. More about bookmarks can be found [here](https://github.com/smart-linux-shell/ishell/blob/main/docs/bookmarking.md).

In this report, user expectations around managing and listing bookmarks for frequently used queries are explored. To understand the best way to present and manage these bookmarks, user interviews were conducted with individuals of varying Linux proficiency. This report summarizes findings and proposed features based on user feedback.

### Contents
- [Research Breakdown](#research-breakdown)
  - [Novice Users](#novice-users)
  - [Regular Users](#regular-users)
  - [Advanced Users](#advanced-users)
- [Feature Proposal](#feature-proposal)
  - [Core Features](#core-features)
  - [Intermediate Features](#intermediate-features)
  - [Advanced Features](#advanced-features)
- [Conclusion](#conclusion)

## Research Breakdown
Users are categorized into three groups based on their proficiency with Linux: novice, regular, and advanced users. The responses from these interviews form the basis for design and feature proposals.

### Novice Users
- **Proportion:** 20% of total interviewees
- **Feedback:**
  - Preferred a **clean and simple** user interface.
  - Wanted to see the bookmarks listed in the **same frame** as the assistant, without needing to switch views or modes.
  - **Pagination** was requested to avoid overwhelming them with a long list of bookmarks.
  - Required only **basic information**, such as the alias and corresponding query. No additional metadata or options were necessary.

### Regular Users
- **Proportion:** 40% of total interviewees
- **Feedback:**
  - Desired the features mentioned by novice users, but with additional **interactivity**.
  - Suggested an **integrated search** or filtering mechanism to quickly locate bookmarks based on keywords or tags.
  - Emphasized the importance of keeping everything in the **same frame** but with the ability to navigate or scroll through results more efficiently.
  - Appreciated having an option to **edit** or **delete** bookmarks directly from the list.
  - Requested that bookmarks be displayed in a **tabular format** with clear separators between the alias and the query.

### Advanced Users
- **Proportion:** 40% of total interviewees
- **Feedback:**
  - Preferred to see the bookmarks in a **separate panel or mode** to avoid cluttering the assistant’s output.
  - Requested advanced features, such as:
    - **Sorting** options (e.g., by popularity, frequency of use, or last accessed).
    - The ability to **nest bookmarks**, allowing related queries to be grouped together and not all showed at once. For example, bookmarks could be structured similarly to GitHub branches, such as `discord/update`, `discord/open`, `network/check`, etc.
      - This feature enables users to avoid scrolling through endless lists or pages, with the ability to browse the bookmarks in a file system organized manner.
      - Some users suggested that nesting with help of an AI would be something they would look up to.
    - **Timestamps** or **last used** information to help prioritize older or frequently accessed bookmarks.
    - Integration with the command line for **advanced filtering** and piping (e.g., `bookmark list | grep 'network'`).
    - A **preview** of bookmark content before execution, to allow advanced users to confirm the query without needing to remember its exact contents.
    - Support for **bulk actions**, such as deleting or renaming multiple bookmarks at once.
  - Some users also suggested enabling **bookmark tagging**, where multiple aliases could be assigned tags like `networking`, `disk management`, etc., for faster searching.


## Feature Proposal

Based on the feedback collected from all user categories, the following comprehensive list of features for the bookmark listing functionality is the following:

### Core Features:
1. **Alias and Query Display:** _[already implemented]_
   - Show the alias along with the corresponding query in a clean, readable format.
   - For example:
     ```
     Alias           | Query
     --------------------------
     search_sysinfo  | How to check system info
     disk_check      | Check disk space
     ```
   
2. **Same Frame Display (Default Mode):** _[already implemented]_
   - For novice and regular users, display the list within the same assistant frame to keep interaction simple.

3. **Pagination:**
   - Implement pagination for novice users to avoid overwhelming them with too many results at once.
   - Example: "Page 1 of 3" with commands for navigation (`n` for next page, `p` for previous page).

### Intermediate Features:
4. **Integrated Search and Filtering:**
   - Allow users to filter or search bookmarks by keywords (e.g., `bookmark list <search_term>`).
   - Example: `bookmark list disk` would show only bookmarks related to disk management.
  
5. **Interactive Listing:**
   - Enable users to select a bookmark from a list using arrows.

6. **Editable Entries:**
   - Enable users to edit selected bookmark aliases or queries directly from the list.

7. **Direct Execution and Deletion:**
   - Provide options to execute or delete a selected bookmark directly from the listing interface.

### Advanced Features:
8. **Separate Bookmark Panel:**
   - Introduce a toggleable, dedicated panel for advanced users, where they can manage bookmarks without cluttering the assistant frame.
   
9. **Sorting Mechanisms:**
   - Provide various sorting options such as:
     - By **popularity** (most frequently used bookmarks at the top).
     - By **last accessed** (recently used bookmarks displayed first).
     - By **alphabetical order**.
   
10. **Nesting Bookmarks:**
    - Allow users to create **nested bookmarks**, structured with paths similar to GitHub branches (e.g., `network/check`, `network/reset`).
    - This could be AI-generated based on the content or user-defined when creating a bookmark.
   
11. **Metadata Display:**
    - Display additional metadata for each bookmark, such as the **last accessed time**, **date created**, and **usage count**.
   
12. **Tagging System:**
    - Support a **tagging** feature, allowing bookmarks to be grouped under relevant tags like `networking`, `system`, etc., to improve searchability.

13. **Advanced Filtering and Piping:**
    - Allow users to use the power of the terminal by piping bookmark lists to other commands for custom sorting or filtering (e.g., `bookmark list | grep 'disk'`).

14. **Bulk Actions:**
    - Enable users to perform bulk operations, such as deleting multiple bookmarks at once or renaming a group of aliases.


## Conclusion

The research revealed significant differences in how novice, regular, and advanced users would like to manage and list bookmarks. By implementing a multi-tiered approach to bookmark functionality, UX can be adapted to the varying needs of users while keeping the interface intuitive for novices and powerful for advanced users. 
