Contributing to In-a-dyn
========================

Thank you for considering contributing back to [Free Software][1]!

There are a few things we would like you to consider when filing an
issue or pull request with this project:

1. If you are filing a bug report or feature request

   Please take the time to check if an issue already has been filed
   matching your problem

2. What version are you running, have you tried the latest release?

   UNIX distributions often package and test software for their
   particular brand.  If you are using a pre-packaged version,
   then please file a bug with that distribution instead.

3. Coding Style

   We use the Linux coding style, like GIT it is becoming a de facto
   standard for C programming

   https://www.kernel.org/doc/Documentation/CodingStyle

4. Logical Change Sets

   Changes should be broken down into logical units that add a feature
   or fix a bug.  Keep changes separate from each other and do not mix a
   bug fix with a whitespace cleanup or a new feature addition.

   This is important not only for readilibity, or for the possibility of
   maintainers to revert changes, but does also increase your chances of
   having a change accepted.

5. Commit messages

   See this helpful guide for how to write simple, readable commit
   messages, or have at least a look at the below example.
   
   http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html


Example
-------

    Capitalized, short (50 chars or less) summary
    
    More detailed explanatory text, if necessary.  Wrap it to about 72
    characters or so.  In some contexts, the first line is treated as the
    subject of an email and the rest of the text as the body.  The blank
    line separating the summary from the body is critical (unless you omit
    the body entirely); tools like rebase can get confused if you run the
    two together.
    
    Write your commit message in the imperative: "Fix bug" and not "Fixed bug"
    or "Fixes bug."  This convention matches up with commit messages generated
    by commands like git merge and git revert.
    
    Further paragraphs come after blank lines.
    
    - Bullet points are okay, too
    
    - Typically a hyphen or asterisk is used for the bullet, followed by a
      single space, with blank lines in between, but conventions vary here
    
    - Use a hanging indent
    
    Signed-off-by: Some Namesson <user@example.org>


[1]: http://www.gnu.org/philosophy/free-sw.en.html

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
