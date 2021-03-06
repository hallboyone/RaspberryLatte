udpr = dsp.UDPReceiver('LocalIPPort', 5005);
setup(udpr);
start_time = 0;
f = figure;
dat_buf = [];
ran = false;
%f.Position = [1224 1 691 1001];
f.Position = [881 1 560 821];
tlim = 10;
colors = ['k', 'r', 'b', 'g'];
brew_idx = 0;
while true
    raw_dat = udpr();
    read_time = posixtime(datetime('now'));
    while isempty(raw_dat)
        raw_dat = udpr();
        if ran && posixtime(datetime('now')) - read_time > 5
            %f = figure;
            dat_buf = [];
            start_time = 0;
            %f.Position = [1224 1 691 1001];
            %f.Position = [881 1 560 821];
            %tlim = 10;
            ran = false;
            brew_idx = brew_idx + 1;
        end
        pause(0.1)
    end
    ran = true;
    dat = ParseLine(convertCharsToStrings(char(raw_dat)));
    if start_time == 0
        start_time = dat.t;
    end
    dat.t = dat.t - start_time;
    dat_buf = [dat_buf, dat];

    if numel(dat_buf)==5
        tlim = plotdat(dat_buf, tlim, colors(mod(brew_idx,4)+1));
        dat_buf = [dat_buf(end)];
    end
end

function dat = ParseLine(ln)
dat = struct;
vars = split(ln, ";");
for i = 1:numel(vars)
    if strlength(vars(i))>1
        name_val_pair = split(vars(i), "=");
        dat.(name_val_pair(1)) = str2double(name_val_pair(2));
    end
end
end

function tlim = plotdat(dat_buf, tlim, c)
limits = struct('temp', [0, 100], 'scale', [0, 50], 'pressure', [0,15],...
    'pump', [0,150], 'heater', [0, 65], 'stage', [0,5]);
if dat_buf(end).t > tlim
    tlim = tlim + 5;
end
fn = fieldnames(dat_buf(1));
for k=2:numel(fn)
    subplot(numel(fn)-1,1,k-1)
    hold on
    plot([dat_buf(:).t], [dat_buf(:).(fn{k})], c)
    title(fn{k})
    xlim([0,tlim])
    ylim(limits.(fn{k}))
end
drawnow
end