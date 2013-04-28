
require('clangcomplete')

AUTOCOMPLETE_MENU_ID = 9

AUTOCOMPLETE_START_POS = 0
AUTOCOMPLETE_RESULTS = {}

AUTOCOMPLETE_MENU_COLUMN_WIDTH = 30

function OnKey(c, shift, ctrl, alt)
	if (editor.Lexer == 3) then  -- c++
		if ((c==32) and (ctrl==true)) then
			AUTOCOMPLETE_START_POS = editor.CurrentPos
			local compilerOptions = {}
			-- append user include paths in every case...
			for token in string.gmatch(props['includePaths.user'], "[^;]+") do
			   table.insert(compilerOptions, "-I" .. token)
			end
			-- append system include paths only if alt is not pressed
			-- should be faster to build the list with alt, it also won't be cluttered with standard library symbols
			if (alt==false) then
				for token in string.gmatch(props['includePaths.system'], "[^;]+") do
				   table.insert(compilerOptions, "-I" .. token)
				end
			end
			-- add preprocessor defines
			for token in string.gmatch(props['defines'], "[^;]+") do
			   table.insert(compilerOptions, "-D" .. token)
			end
			-- make sure the file path is not empty or clang will report an error
			currentFilePath = props['FilePath']
			if currentFilePath == "" or string.sub(currentFilePath, -1, -1) == "\\" or string.sub(currentFilePath, -1, -1) == "/" then
				currentFilePath = currentFilePath .. "Untitled.cpp"
			end

			AUTOCOMPLETE_RESULTS = clangcomplete.complete(currentFilePath, editor.CurrentPos, tostring(editor:GetText()), compilerOptions)

			local suggestions = {}
			local replacement
			local description
			for _,completionResult in ipairs(AUTOCOMPLETE_RESULTS) do
				description = {}
				replacement = ""
				for _, completionChunk in ipairs(completionResult) do
					if completionChunk[1] == 1 then
					    -- this is the token that should be inserted as a result of autocompletion
						replacement = completionChunk[2]
					end
					table.insert(description, completionChunk[2])
				end
				-- format the list into 2 columns, try to make the first one fixed length so it looks nice with monospace font
				fillerLen = AUTOCOMPLETE_MENU_COLUMN_WIDTH - #replacement
				if fillerLen < 0 then fillerLen = 0 end
				table.insert(suggestions, replacement .. string.rep(" ", fillerLen) .. " " .. table.concat(description))
			end
			-- unfortunately, this needs to be sorted and the order is case sensitive or else the user list will not work properly when the user enters additional characters
			-- libclang only sorts the results in case insensitive way, so this either has to be done in C or inside Lua script
			table.sort(suggestions, function (a, b)
				return a < b
			end)
			if #suggestions > 0 then
				local sep = '•'
				local sep_tmp = editor.AutoCSeparator
				editor.AutoCSeparator = string.byte(sep)
				editor:UserListShow(AUTOCOMPLETE_MENU_ID, table.concat(suggestions, sep))
				editor.AutoCSeparator = sep_tmp
				return true;
			else
				return false;
			end
		end
		
	end
end

function OnUserListSelection(t,str)
  if t == AUTOCOMPLETE_MENU_ID then 
		editor:SetSel(AUTOCOMPLETE_START_POS, editor.CurrentPos)
		editor:ReplaceSel(string.sub(str, 1, string.find(str, " ") - 1))
     return true
  else
     return false
  end
end
