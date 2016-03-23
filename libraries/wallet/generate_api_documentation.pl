#! /usr/bin/perl

use Text::Wrap;
use IO::File;

require 'doxygen/perlmod/DoxyDocs.pm';

my($outputFileName) = @ARGV;
die "usage: $0 output_file_name" unless $outputFileName;
my $outFile = new IO::File($outputFileName, "w")
  or die "Error opening output file: $!";

my $fileHeader = <<'END';
/** GENERATED FILE **/
#include <set>
#include <steemit/wallet/api_documentation.hpp>
#include <steemit/wallet/wallet.hpp>

namespace steemit { namespace wallet {
   namespace detail
   {
      struct api_method_name_collector_visitor
      {
         std::set<std::string> method_names;

         template<typename R, typename... Args>
         void operator()( const char* name, std::function<R(Args...)>& memb )
         {
            method_names.emplace(name);
         }
      };
   }

   api_documentation::api_documentation()
   {
END
$outFile->print($fileHeader);

for my $class (@{$doxydocs->{classes}})
{
  if ($class->{name} eq 'steemit::wallet::wallet_api')
  {
    for my $member (@{$class->{public_methods}->{members}})
    {
      if ($member->{kind} eq 'function')
      {
        my @params = map { join(' ', cleanupDoxygenType($_->{type}), $_->{declaration_name}) } @{$member->{parameters}};
        my $briefDescription = sprintf("%40s %s(%s)\n", cleanupDoxygenType($member->{type}), $member->{name}, join(', ', @params));
        my $escapedBriefDescription = "\"" . escapeStringForC($briefDescription) . "\"";
        my %paramInfo = map { $_->{declaration_name} => { type => $_->{type}} } @{$member->{parameters}};
        my $escapedDetailedDescription = "\"\"\n";
        if ($member->{detailed}->{doc})
        {
          my $docString = formatDocComment($member->{detailed}->{doc}, \%paramInfo);
          for my $line (split(/\n/, $docString))
          {
            $escapedDetailedDescription .=  "                \"" .  escapeStringForC($line . "\n") . "\"\n";
          }
        }
        my $codeFragment = <<"END";
     {
        method_description this_method;
        this_method.method_name = "$member->{name}";
        this_method.brief_description = $escapedBriefDescription;
        this_method.detailed_description = $escapedDetailedDescription;
        method_descriptions.insert(this_method);
     }

END
        $outFile->print($codeFragment);
      }
    }
  }
}

my $fileFooter = <<'END';
      fc::api<wallet_api> tmp;
      detail::api_method_name_collector_visitor visitor;
      tmp->visit(visitor);
      for (auto iter = method_descriptions.begin(); iter != method_descriptions.end();)
        if (visitor.method_names.find(iter->method_name) == visitor.method_names.end())
          iter = method_descriptions.erase(iter);
        else
          ++iter;
   }

} } // end namespace steemit::wallet
END
$outFile->print($fileFooter);
$outFile->close();

sub cleanupDoxygenType
{
  my($type) = @_;
  $type =~ s/< /</g;
  $type =~ s/ >/>/g;
  return $type;
}

sub formatDocComment
{
  my($doc, $paramInfo) = @_;

  my $bodyDocs = '';
  my $paramDocs = '';
  my $returnDocs = '';

  for (my $i = 0; $i < @{$doc}; ++$i)
  {
    if ($doc->[$i] eq 'params')
    {
      $paramDocs .= "Parameters:\n";
      @parametersList = @{$doc->[$i + 1]};
      for my $parameter (@parametersList)
      {
        my $declname = $parameter->{parameters}->[0]->{name};
        my $decltype = cleanupDoxygenType($paramInfo->{$declname}->{type});
        $paramDocs .= Text::Wrap::fill('    ', '        ', "$declname: " . formatDocComment($parameter->{doc}) . " (type: $decltype)") . "\n";
      }
      ++$i;
    }
    elsif ($doc->[$i]->{return})
    {
      $returnDocs .= "Returns\n";
      $returnDocs .= Text::Wrap::fill('    ','        ', formatDocComment($doc->[$i]->{return})) . "\n";
    }
    else
    {
      my $docElement = $doc->[$i];
      if ($docElement->{type} eq 'text' or $docElement->{type} eq 'url')
      {
        $bodyDocs .= $docElement->{content};
      }
      elsif ($docElement->{type} eq 'parbreak')
      {
        $bodyDocs .= "\n\n";
      }
      elsif ($docElement->{type} eq 'style' and $docElement->{style} eq 'code')
      {
        $bodyDocs .= "'";
      }
    }
  }

  $bodyDocs =~ s/^\s+|\s+$//g;
  $bodyDocs = Text::Wrap::fill('', '', $bodyDocs);

  $paramDocs =~ s/^\s+|\s+$//g;
  $returnDocs =~ s/^\s+|\s+$//g;

  my $result = Text::Wrap::fill('', '', $bodyDocs);
  $result .= "\n\n" . $paramDocs if $paramDocs;
  $result .= "\n\n" . $returnDocs if $returnDocs;

  return $result;
}

sub escapeCharForCString
{
  my($char) = @_;
  return "\\a" if $char eq "\x07";
  return "\\b" if $char eq "\x08";
  return "\\f" if $char eq "\x0c";
  return "\\n" if $char eq "\x0a";
  return "\\r" if $char eq "\x0d";
  return "\\t" if $char eq "\x09";
  return "\\v" if $char eq "\x0b";
  return "\\\"" if $char eq "\x22";
  return "\\\\" if $char eq "\x5c";
  return $char;
}

sub escapeStringForC
{
  my($str) = @_;
  return join('', map { escapeCharForCString($_) } split('', $str));
}


