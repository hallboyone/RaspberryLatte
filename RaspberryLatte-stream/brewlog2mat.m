function M = brewlog2mat(filename)
try
    fid = fopen(filename);
catch E
    disp("Could not open file")
    disp(E)
    M = [];
    return
end
tline = fgetl(fid);
tlines = cell(0,1);
while ischar(tline)
    tlines{end+1,1} = tline;
    tline = fgetl(fid);
end
fclose(fid);

num_fields = sum(tlines{1}=='=');
num_lines = numel(tlines);
M = zeros(num_lines, num_fields);
for i=1:num_lines
    M(i,:) = ln2vec(tlines{i});
end
M(:,1) = M(:,1) - M(1,1);
end

function v = ln2vec(ln)
fields = split(ln, ";");
fields = fields(1:end-1);
v = zeros(1,numel(fields));
for i = 1:numel(fields)
    name_val_pair = split(fields(i), "=");
    v(i) = str2double(name_val_pair(2));
end
end