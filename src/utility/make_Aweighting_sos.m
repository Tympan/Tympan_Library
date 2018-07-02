
% define transfer function of A and C weighting filters
% Matlab central: https://www.mathworks.com/matlabcentral/fileexchange/69-octave
%    [1] IEC/CD 1672: Electroacoustics-Sound Level Meters, Nov. 1996.

% Definition of analog A-weighting filter according to IEC/CD 1672.
f1 = 20.598997; f2 = 107.65265; f3 = 737.86223;f4 = 12194.217;
A1000 = 1.9997;
A_NUMs = [ (2*pi*f4)^2*(10^(A1000/20)) 0 0 0 0 ];
A_DENs = conv([1 +4*pi*f4 (2*pi*f4)^2],[1 +4*pi*f1 (2*pi*f1)^2]);
A_DENs = conv(conv(A_DENs,[1 2*pi*f3]),[1 2*pi*f2]);

% Definition of analog C-weighting filter according to IEC/CD 1672.
f1 = 20.598997; f4 = 12194.217;
C1000 = 0.0619;
C_NUMs = [ (2*pi*f4)^2*(10^(C1000/20)) 0 0 ];
C_DENs = conv([1 +4*pi*f4 (2*pi*f4)^2],[1 +4*pi*f1 (2*pi*f1)^2]);

%define the sample rates that we want to use
all_fs_Hz = [16000 22050 32000 44100 48000 88200 96000];

%loop over each sample rate and compute the A- and C-weighting filters
%as IIR filters, specifically as second-order-sectcions for implementing
%as biquad IIR filters on the Tympan
all_A_sos={}; all_C_sos={};
for Ifs = 1:length(all_fs_Hz)
    fs_Hz = all_fs_Hz(Ifs);
    
    %calc A-weighting
    [b,a] = bilinear(A_NUMs,A_DENs,fs_Hz);
    sos = tf2sos(b,a);
    all_A_sos{Ifs} = sos;
    
    %calc C-weighting
    [b,a] = bilinear(C_NUMs,C_DENs,fs_Hz);
    sos = tf2sos(b,a);
    all_C_sos{Ifs} = sos;
end

%% write in C++ form for use in the Arduino IDE
%% print text to screen for copying into C++
diary off
!del FreqWeighting_IEC1672_coeff.h
diary('FreqWeighting_IEC1672_coeff.h')
%disp(['#ifndef _FreqWeighting_IEC1672_h']);
%disp(['#define _FreqWeighting_IEC1672_h']);
disp(['    //These filter coefficients were written by the Matlab code "make_A_C_weighting_sos.m']);
%disp([' ']);
%disp(['class FreqWeighting_IEC1672 {']);
%disp(['  public:']);
disp(['    #define N_FS_HZ ('  num2str(length(all_fs_Hz)) ')']);
disp(['    const int N_all_fs_Hz = N_FS_HZ;']);
disp(['    float32_t all_fs_Hz[N_FS_HZ]={' ...
    sprintf('%i, ', all_fs_Hz(1:end-1)) ...
    num2str(all_fs_Hz(end)) '};']);

disp([' ']);
for Itype=1:2
    if Itype==1
        all_sos = all_A_sos;
        variable_name1 = 'Aweight_N_SOS_PER_FILTER';
        comment_txt = '// Define the A-weighting coefficients';
        variable_name2 = 'all_A_matlab_sos';
    elseif Itype==2
        all_sos = all_C_sos;
        variable_name1 = 'Cweight_N_SOS_PER_FILTER';
        comment_txt = '// Define the C-weighting coefficients';
        variable_name2 = 'all_C_matlab_sos';
    end
    
    %write the header comment
    disp(['    ' comment_txt]);
    
    %write number of second order sections used per filter
    disp(['    static const int ' variable_name1 ' = ' num2str(size(all_sos{1},1)) ';']);
    
    %write the second order section coefficients
    disp(['    float32_t ' variable_name2 '[N_FS_HZ][' variable_name1 '*6] = {  ']);
    for Ifilt=1:length(all_sos)
        sos = all_sos{Ifilt};
        disp(['      { ']);
        for I=1:size(sos,1)
            disp(sprintf('       %.10e, %.10e, %.10e, %.10e, %.10e, %.10e, \\',sos(I,:)));
        end
        if Ifilt < length(all_sos); disp(['      }, ']); else; disp(['      } ']);  end
    end
    disp(['    };']);
    disp([' ']);
    
end
%disp('};'); % close off the class
%disp([' ']);

%disp(['#endif']);

diary off

return
%% write the

